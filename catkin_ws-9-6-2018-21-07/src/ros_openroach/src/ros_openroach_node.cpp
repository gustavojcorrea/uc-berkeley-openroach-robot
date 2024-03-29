/*******************************************************************************
 * rc_balance.c
 *
 * James Strawson 2017
 * Reference solution for balancing EduMiP
 *
 * edummip_ros_balance.cpp based on rc_balance.c by James Strawson
 *
 * Reference solution for balancing EduMiP
 *
 * 2017-02-13 Louis Whticomb modified to tx status and rx commands via ROS topics
 *            RX velocity command on geometry_msgs::Twist topic /edumip/cmd
 *            RX edumit state on edumip_balance_ros::EduMIP_State topic /edumip/state
 * 
 *******************************************************************************/

//
// 2016-01-14 LLW roboticscape-usefulincludes.h and roboticscape.h and the
//                include files they reference define external C functions
//                (not C++ functions) that are seprately compiled and linked
//                into libroboticscape.so.  The robotics cape installer puts
//                this file in
//                  /usr/lib/libroboticscape.so 
//                and puts these include files in /usr/include:
//                  /usr/include/roboticscape.h
//                  /usr/include/roboticscape-defs.h
//                  /usr/include/roboticscape-usefulincludes.h
//                where they are found by the ROS cmake system
//

// 2016-01-14 LLW since these libraries are compiled for C, not C++, we need
//                to specify extern "C" to allow us to access them from this C++
//                program.
extern "C"
{
#include "rc_usefulincludes.h"
}
extern "C"
{  
#include "roboticscape.h"
}

// 2016-01-14 LLW This header file is located in this package in
//                edumip_balance_ros/include/balance_config.h
#include "balance_config.h"
#include "openroach_class.h"
// 2017-01-08 LLW Header files for ROS and system time
#include <ros/ros.h>
// 2017-01-08 LLW Header files for ROS string messages
#include "std_msgs/String.h"
// 2017-01-10 LLW Header files for twist msh
#include "geometry_msgs/Twist.h"
#include "geometry_msgs/PoseStamped.h"
//#include <tf/transform_broadcaster.h>
// 2017-02-13 LLW Added edumip state message from the package edumip_msg
#include "edumip_msgs/EduMipState.h"

#include <unistd.h>

// 2017-01-09 LLW global variables
float   gamma_dot_desired = 0.0;
float   phi_dot_desired   = 0.0;
float   time_last_good_ros_command_sec = 0.0;

// 2017-02-22 LLWnavigation 
float   body_frame_easting  = 0.0;
float   body_frame_northing = 0.0;
float   body_frame_heading  = 0.0;

// 2017-02-17 LLW Global controller vars
static int inner_saturation_counter = 0; 
float dutyL;  // left  motor PWM duty cycle
float dutyR;  // right motor PWM duty cycle
float DUTY_L_GLOBAL;
float DUTY_R_GLOBAL;

// global variables
ros::Publisher              state_publisher;


/*******************************************************************************
 * drive_mode_t
 *
 * NOVICE: Drive rate and turn rate are limited to make driving easier.
 * ADVANCED: Faster drive and turn rate for more fun.
 *******************************************************************************/
typedef enum drive_mode_t{
  NOVICE,
  ADVANCED
}drive_mode_t;

/*******************************************************************************
 * arm_state_t
 *
 * ARMED or DISARMED to indicate if the controller is running
 *******************************************************************************/
typedef enum arm_state_t{
  ARMED,
  DISARMED
}arm_state_t;

/*******************************************************************************
 * setpoint_t
 *	
 * Controller setpoint written to by setpoint_manager and read by the controller.
 *******************************************************************************/
typedef struct setpoint_t{
  arm_state_t arm_state;	// see arm_state_t declaration
  drive_mode_t drive_mode;// NOVICE or ADVANCED
  float theta;			// body lean angle (rad)
  float phi;				// wheel position (rad)
  float phi_dot;			// rate at which phi reference updates (rad/s)
  float gamma;			// body turn angle (rad)
  float gamma_dot;		// rate at which gamma setpoint updates (rad/s)
}setpoint_t;

/*******************************************************************************
 * core_state_t
 *
 * This is the system state written to by the balance controller.	
 *******************************************************************************/
typedef struct core_state_t{
  float wheelAngleR;	// wheel rotation relative to body
  float wheelAngleL;
  float theta; 		// body angle radians
  float phi;			// average wheel angle in global frame
  float gamma;		// body turn (yaw) angle radians
  float vBatt; 		// battery voltage 
  float d1_u;			// output of balance controller D1 to motors
  float d2_u;			// output of position controller D2 (theta_ref)
  float d3_u;			// output of steering controller D3 to motors
  float mot_drive;	// u compensated for battery voltage
} core_state_t;

/*******************************************************************************
 * Local Function declarations	
 *******************************************************************************/
// IMU interrupt routine
void balance_controller(); 
// threads
void* setpoint_manager(void* ptr);
void* battery_checker(void* ptr);
void* printf_loop(void* ptr);
// regular functions
int zero_out_controller();
int disarm_controller();
int arm_controller();
int wait_for_starting_condition();
void on_pause_press();
void on_mode_release();
int blink_green();
int blink_red();

/*******************************************************************************
 * Global Variables				
 *******************************************************************************/
core_state_t cstate;
setpoint_t setpoint;
rc_filter_t D1, D2, D3;
rc_imu_data_t imu_data;


/*******************************************************************************
 * shutdown_signal_handler(int signo)
 *
 * catch Ctrl-C signal and change system state to EXITING
 * all threads should watch for get_state()==EXITING and shut down cleanly
 *******************************************************************************/
void ros_compatible_shutdown_signal_handler(int signo)
{
  if (signo == SIGINT)
    {
      rc_set_state(EXITING);
      ROS_INFO("\nReceived SIGINT Ctrl-C.");
      ros::shutdown();
    }
  else if (signo == SIGTERM)
    {
      rc_set_state(EXITING);
      ROS_INFO("Received SIGTERM.");
      ros::shutdown();
    }
}


void CmdCallback(const geometry_msgs::Twist::ConstPtr& cmd_vel_twist)
{

  // assign the commands if the array is of the correct length
  phi_dot_desired   = cmd_vel_twist->linear.x;
  gamma_dot_desired = cmd_vel_twist->angular.z;

  time_last_good_ros_command_sec = ros::Time::now().toSec();

  return;
}

void pose_callback(const geometry_msgs::PoseStamped::ConstPtr& pose){
        float x_pos = pose->pose.position.x;
        float y_pos = pose->pose.position.y;
        float z_pos = pose->pose.position.z;

        //ROS_INFO("x, y, z= %1.2f %1.2f %1.2f" , x_pos, y_pos, z_pos);


}


//CONSTRUCTOR:  this will get called whenever an instance of this class is created
// want to put all dirty work of initializations here
// odd syntax: have to pass nodehandle pointer into constructor for constructor to build subscribers, etc
OpenRoachClass::OpenRoachClass(ros::NodeHandle* nodehandle):nh_(*nodehandle)
{ // constructor
    ROS_INFO("in class constructor of OpenRoachClassClass");
    initializeSubscribers(); // package up the messy work of creating subscribers; do this overhead in constructor
    initializePublishers();
    initializeServices();
    
    //initialize variables here, as needed
    val_to_remember_=0.0; 
    DUTY_MAX = 0.6;
    WORLD_FRAME_NAME = "WORLD";
    BODY_FRAME_NAME = "OPENROACH";
    
    DUTY_L = 0.2;
    DUTY_R = 0.2;
    //rc_set_motor(MOTOR_CHANNEL_L, MOTOR_POLARITY_L * DUTY_R);
    //rc_set_motor(MOTOR_CHANNEL_R, MOTOR_POLARITY_R * DUTY_R);
    // can also do tests/waits to make sure all required services, topics, etc are alive
}

//member helper function to set up subscribers;
// note odd syntax: &OpenRoachClassClass::subscriberCallback is a pointer to a member function of OpenRoachClassClass
// "this" keyword is required, to refer to the current instance of OpenRoachClassClass
void OpenRoachClass::initializeSubscribers()
{
    ROS_INFO("Initializing Subscribers");
    // add more subscribers here, as needed
    tag_pose_sub = nh_.subscribe("/visp_auto_tracker/object_position", 1, &OpenRoachClass::tag_pose_callback,this);
}

//member helper function to set up services:
// similar syntax to subscriber, required for setting up services outside of "main()"
void OpenRoachClass::initializeServices()
{
    ROS_INFO("Initializing Services");
    // add more services here, as needed
}

//member helper function to set up publishers;
void OpenRoachClass::initializePublishers()
{
    ROS_INFO("Initializing Publishers");
    //minimal_publisher_ = nh_.advertise<std_msgs::Float32>("exampleMinimalPubTopic", 1, true); 
    //add more publishers, as needed
    // note: COULD make minimal_publisher_ a public member function, if want to use it within "main()"
}


float max(float a, float b) {
  if (a >= b) return a;
  return b;
}

void OpenRoachClass::tag_pose_callback(const geometry_msgs::PoseStamped::ConstPtr& pose){
        x_pos_tag = pose->pose.position.x;
        y_pos_tag = pose->pose.position.y;
        z_pos_tag = pose->pose.position.z;
	
	//tf::Quaternion q(pose->pose.orientation.x, pose->pose.orientation.y, pose->pose.orientation.z, pose->pose.orientation.w);
        //tf::Matrix3x3 m(q);
        //m.getRPY(roll_tag, pitch_tag, yaw_tag);

        //ROS_INFO("x, y, z= %1.2f %1.2f %1.2f" , x_pos_tag, y_pos_tag, z_pos_tag);

	//static tf::TransformBroadcaster br;
	//tf::Transform transform;
	//transform.setOrigin( tf::Vector3(x_pos_tag, y_pos_tag, z_pos_tag) );
	//tf::Quaternion q;
	//q.setRPY(0, 0, msg->theta);
	//transform.setRotation(q);
	//br.sendTransform(tf::StampedTransform(transform, ros::Time::now(), WORLD_FRAME_NAME, BODY_FRAME_NAME));

/*
	float error_x = 0 - x_pos_tag;
	
	//Update propotional term for x axis
	float pX = error_x * 0.2;

	// Update Derivative term for x axis
	float dX = (error_x - prevXerror) * .4;
	prevXerror = error_x;

	// combine P and D
	float total_PD = pX;

	if(error_x > 0)
	{
		DUTY_L -= total_PD;
		DUTY_R += total_PD;
	}
	else
	{
		DUTY_L += total_PD;
                DUTY_R -= total_PD;
	}


	if (DUTY_L > DUTY_MAX)
        {
                DUTY_L = DUTY_MAX;
        }
        else if (DUTY_L < -DUTY_MAX)
        {
		DUTY_L = -DUTY_MAX;
        }
	
	if (DUTY_R > DUTY_MAX)
        {       
                DUTY_R = DUTY_MAX;
        }
        else if (DUTY_R < -DUTY_MAX)
        {       
                DUTY_R = -DUTY_MAX;
        }
*/
  float line_pos = x_pos_tag;
  float TURN_SENS_INNER = 1.5;
  float SPEED_PWM = 0.5;

  if (line_pos < 0) {
    DUTY_R = SPEED_PWM;
    DUTY_L = max(SPEED_PWM * (1.0F + TURN_SENS_INNER * line_pos * 2.0F),0);
  }

  if (line_pos >= 0) {
    DUTY_L = SPEED_PWM;
    DUTY_R = max(SPEED_PWM * (1.0F - TURN_SENS_INNER * line_pos * 2.0F),0);
  }
	DUTY_L_GLOBAL = DUTY_L;
	DUTY_R_GLOBAL = DUTY_R;
	//rc_set_motor(MOTOR_CHANNEL_L, MOTOR_POLARITY_L * DUTY_R);
  	//rc_set_motor(MOTOR_CHANNEL_R, MOTOR_POLARITY_R * DUTY_R);

	ROS_INFO("x, y, z= %1.4f %1.4f %1.4f DUTY_L, DUTY_R= %1.4f %1.4f " , x_pos_tag, y_pos_tag, z_pos_tag,DUTY_L, DUTY_R);
}

void steering_dumb(const geometry_msgs::PoseStamped::ConstPtr& pose){
  float x_pos_tag = pose->pose.position.x;
  float y_pos_tag = pose->pose.position.y;
  float z_pos_tag = pose->pose.position.z;
  float line_pos = x_pos_tag;
  float TURN_SENS_INNER = 1.5;
  float SPEED_PWM = 0.7;
  float DUTY_L = DUTY_L_GLOBAL;
  float DUTY_R = DUTY_R_GLOBAL;

  if (line_pos < 0) {
    DUTY_R = SPEED_PWM;
    DUTY_L = max(SPEED_PWM * (1.0F + TURN_SENS_INNER * line_pos * 2.0F),0);
  }

  if (line_pos >= 0) {
    DUTY_L = SPEED_PWM;
    DUTY_R = max(SPEED_PWM * (1.0F - TURN_SENS_INNER * line_pos * 2.0F),0);
  }
        DUTY_L_GLOBAL = DUTY_L;
        DUTY_R_GLOBAL = DUTY_R;
        //rc_set_motor(MOTOR_CHANNEL_L, MOTOR_POLARITY_L * DUTY_R);
        //rc_set_motor(MOTOR_CHANNEL_R, MOTOR_POLARITY_R * DUTY_R);

        //ROS_INFO("x, y, z= %1.4f %1.4f %1.4f DUTY_L, DUTY_R= %1.4f %1.4f " , x_pos_tag, y_pos_tag, z_pos_tag,DUTY_L, DUTY_R);
}

void tag_pose_cb(const geometry_msgs::PoseStamped::ConstPtr& pose){
        float x_pos_tag = pose->pose.position.x;
        float y_pos_tag = pose->pose.position.y;
        float z_pos_tag = pose->pose.position.z;
	float DUTY_L = DUTY_L_GLOBAL;
	float DUTY_R = DUTY_R_GLOBAL;
	float DUTY_MAX = .3;
	float kp = 0.3; // proportional constant

	float SPEED_PWM = 3;
	float TURN_SENS_INNER = 1.5;
	float TURN_SENS_OUTER = 0.5;

	float pwm_outer = SPEED_PWM*(1.0F + TURN_SENS_OUTER*abs(x_pos_tag)*2.0F);
        float pwm_inner = SPEED_PWM*(1.0F - TURN_SENS_INNER*abs(x_pos_tag)*2.0F);


/*
	// Write to the appropriate PWM pins
    if(x_pos_tag < 0) {
        DUTY_L = pwm_outer;
	//m1_fwd.write(pwm_outer);
        //m1_back.write(0);
        if(pwm_inner >= 0) {
            DUTY_R = pwm_inner;
		//m2_fwd.write(pwm_inner);
            //m2_back.write(0);
        } else {
            DUTY_R = 0;
	    //m2_fwd.write(0);
            //m2_back.write(pwm_inner*-1.0F);
        }
    }
    if(x_pos_tag >= 0) {
        DUTY_R = pwm_outer;
        //m1_fwd.write(pwm_outer);
        //m1_back.write(0);
        if(pwm_inner >= 0) {
            DUTY_L = pwm_inner;
                //m2_fwd.write(pwm_inner);
            //m2_back.write(0);
        } else {
            DUTY_L = 0;
            //m2_fwd.write(0);
            //m2_back.write(pwm_inner*-1.0F);
        }
    }
*/
/*
    if(x_pos_tag >= 0) {
        m2_fwd.write(pwm_outer);
        m2_back.write(0);
        if(pwm_inner >= 0) {
            m1_fwd.write(pwm_inner);
            m1_back.write(0);
        } else {
            m1_fwd.write(0);
            m1_back.write(pwm_inner*-1.0F);
        }
    }
*/

	float error_x = 0 - x_pos_tag;
	float prevXerror = 0;
        //Update propotional term for x axis
        float pX = error_x * kp;

        // Update Derivative term for x axis
        float dX = (error_x - prevXerror) * .4;
        prevXerror = error_x;

        // combine P and D
        float total_PD = pX;
	
	DUTY_L += total_PD;

/*
        if(error_x > 0)
        {
                DUTY_L -= total_PD;
                //DUTY_R += total_PD;
        }
        else if (error_x < 0)
        {
                DUTY_L += total_PD;
                //DUTY_R -= total_PD;
        }
*/	

        if (DUTY_L > DUTY_MAX)
        {
                DUTY_L = DUTY_MAX;
        }
        else if (DUTY_L < -DUTY_MAX)
        {
                DUTY_L = -DUTY_MAX;
        }

        if (DUTY_R > DUTY_MAX)
        {
                DUTY_R = DUTY_MAX;
        }
        else if (DUTY_R < -DUTY_MAX)
        {
                DUTY_R = -DUTY_MAX;
        }

        DUTY_L_GLOBAL = DUTY_L;
        DUTY_R_GLOBAL = DUTY_R;

	//ROS_INFO("x, y, z= %1.4f %1.4f %1.4f DUTY_L, DUTY_R= %1.4f %1.4f " , x_pos_tag, y_pos_tag, z_pos_tag,DUTY_L, DUTY_R);
}
/*******************************************************************************
 * main()

 *
 * Initialize the filters, IMU, threads, & wait untill shut down
 *******************************************************************************/
int main(int argc, char** argv)  
{

  // Announce this program to the ROS master as a "node" called "edumip_balance_ros_node"
  ros::init(argc, argv, "edumip_balance_ros_node");

  // Start the node resource managers (communication, time, etc)
  ros::start();

  // Broadcast a simple log message
  ROS_INFO("File %s compiled on %s %s.",__FILE__, __DATE__, __TIME__);

  // Create nodehandle
  ros::NodeHandle edumip_node;

  // Advertise the topics this node will publish
  state_publisher = edumip_node.advertise<edumip_msgs::EduMipState>("edumip/state", 10);

  DUTY_L_GLOBAL = .3;
  DUTY_R_GLOBAL = .3;  
  //`OpenRoachClass roach(&edumip_node);

  // subscribe the function CmdCallback to the topuc edumip/cmd
  ros::Subscriber sub_cmd = edumip_node.subscribe("edumip/cmd", 10, CmdCallback);
  //ros::Subscriber sub = edumip_node.subscribe("/visp_auto_tracker/object_position", 1, pose_callback);
  //ros::Subscriber tag_pose_sub = edumip_node.subscribe("/visp_auto_tracker/object_position", 1, tag_pose_cb);
  ros::Subscriber tag_pose_sub = edumip_node.subscribe("/visp_auto_tracker/object_position", 1, steering_dumb);

  if(rc_initialize()<0)
    {
      ROS_INFO("ERROR: failed to initialize cape.");
      return -1;
    }

  rc_set_led(RED,1);
  rc_set_led(GREEN,0);
  rc_set_state(UNINITIALIZED);

  // make sure setpoint starts at normal values
  setpoint.arm_state = DISARMED;
  setpoint.drive_mode = NOVICE;
	
  D1=rc_empty_filter();
  D2=rc_empty_filter();
  D3=rc_empty_filter();
	
  // set up D1 Theta controller
  float D1_num[] = D1_NUM;
  float D1_den[] = D1_DEN;
  if(rc_alloc_filter_from_arrays(&D1,D1_ORDER, DT, D1_num, D1_den)){
    ROS_INFO("ERROR in rc_balance, failed to make filter D1\n");
    return -1;
  }
  D1.gain = D1_GAIN;
  rc_enable_saturation(&D1, -1.0, 1.0);
  rc_enable_soft_start(&D1, SOFT_START_SEC);
	
  // set up D2 Phi controller
  float D2_num[] = D2_NUM;
  float D2_den[] = D2_DEN;
  if(rc_alloc_filter_from_arrays(&D2, D2_ORDER, DT, D2_num, D2_den)){
    ROS_INFO("ERROR in rc_balance, failed to make filter D2\n");
    return -1;
  }
  D2.gain = D2_GAIN;
  rc_enable_saturation(&D2, -THETA_REF_MAX, THETA_REF_MAX);
  rc_enable_soft_start(&D2, SOFT_START_SEC);
	
  ROS_INFO("Inner Loop controller D1:\n");
  rc_print_filter(D1);
  ROS_INFO("\nOuter Loop controller D2:\n");
  rc_print_filter(D2);
	
  // set up D3 gamma (steering) controller
  if(rc_pid_filter(&D3, D3_KP, D3_KI, D3_KD, 4*DT, DT)){
    ROS_INFO("ERROR in rc_balance, failed to make steering controller\n");
    return -1;
  }
  rc_enable_saturation(&D3, -STEERING_INPUT_MAX, STEERING_INPUT_MAX);

  // set up button handlers
  rc_set_pause_pressed_func(&on_pause_press);
  rc_set_mode_released_func(&on_mode_release);
	
  // start a thread to slowly sample battery 
  pthread_t  battery_thread;
  pthread_create(&battery_thread, NULL, battery_checker, (void*) NULL);
  // wait for the battery thread to make the first read
  while(cstate.vBatt==0 && rc_get_state()!=EXITING) rc_usleep(1000);
	
  // start printf_thread if running from a terminal
  // if it was started as a background process then don't bother
  // 2017-11-19 LLW comment out isatty() call that prevents this program from
  //                being run from a launch file
  //  if(isatty(fileno(stdout))){
    pthread_t  printf_thread;
    pthread_create(&printf_thread, NULL, printf_loop, (void*) NULL);
   //  }
	
  // set up IMU configuration
  rc_imu_config_t imu_config = rc_default_imu_config();
  imu_config.dmp_sample_rate = SAMPLE_RATE_HZ;
  imu_config.orientation = ORIENTATION_Y_UP;

  // start imu
  if(rc_initialize_imu_dmp(&imu_data, imu_config)){
    ROS_INFO("ERROR: can't talk to IMU, all hope is lost\n");
    rc_blink_led(RED, 5, 5);
    return -1;
  }

  // 2017-01-10 overide the robotics cape default signal handleers with
  // one that is ros compatible
  signal(SIGINT,  ros_compatible_shutdown_signal_handler);	
  signal(SIGTERM, ros_compatible_shutdown_signal_handler);	
	
  // start balance stack to control setpoints
  pthread_t  setpoint_thread;
  pthread_create(&setpoint_thread, NULL, setpoint_manager, (void*) NULL);

  // this should be the last step in initialization 
  // to make sure other setup functions don't interfere
  rc_set_imu_interrupt_func(&balance_controller);
	
  // start in the RUNNING state, pressing the puase button will swap to 
  // the PUASED state then back again.
  ROS_INFO("\nHold your MIP upright to begin balancing\n");
  rc_set_state(RUNNING);
	
  // chill until something exits the program
  // while(rc_get_state()!=EXITING){
  //   rc_usleep(10000);
  // }

  // Process ROS callbacks until receiving a SIGINT (ctrl-c)
  ros::spin();

  // news
  ROS_INFO("Exiting!");

  // shut down the pthreads
  rc_set_state(EXITING);

  // Stop the ROS node's resources
  ros::shutdown();
	
  // cleanup
  rc_free_filter(&D1);
  rc_free_filter(&D2);
  rc_free_filter(&D3);
  rc_power_off_imu();
  rc_cleanup();
  return 0;
}

/*******************************************************************************
 * void* setpoint_manager(void* ptr)
 *
 * This thread is in charge of adjusting the controller setpoint based on user
 * inputs from dsm radio control. Also detects pickup to control arming the
 * controller.
 *******************************************************************************/
void* setpoint_manager(void* ptr)
{
  float drive_stick, turn_stick; // dsm input sticks

  // 2017-01-09 LLW time computation
  int stat;
  float   time_now_sec;
  float   time_since_last_good_ros_command_sec; 

  // wait for IMU to settle
  disarm_controller();
  rc_usleep(2500000);
  rc_set_state(RUNNING);
  rc_set_led(RED,0);
  rc_set_led(GREEN,1);
	
  while(rc_get_state()!=EXITING)
    {
      // sleep at beginning of loop so we can use the 'continue' statement
      rc_usleep(1000000/SETPOINT_MANAGER_HZ); 
		
      // nothing to do if paused, go back to beginning of loop
      if(rc_get_state() != RUNNING) continue;

      // if we got here the state is RUNNING, but controller is not
      // necessarily armed. If DISARMED, wait for the user to pick MIP up
      // which will we detected by wait_for_starting_condition()
      if(setpoint.arm_state == DISARMED){
	if(wait_for_starting_condition()==0){
	  zero_out_controller();
	  arm_controller();
	} 
	else continue;
      }

      // 2017-01-08 LLW Log the time of most recent good udp packet
      time_now_sec = ros::Time::now().toSec();		  
      time_since_last_good_ros_command_sec = time_now_sec - time_last_good_ros_command_sec;

      // if we have received commands within the last second, update the setpoint rates
      if(time_since_last_good_ros_command_sec < 1.0)
	{
		  
	  // saturate the inputs to avoid possible erratic behavior
	  rc_saturate_float(&phi_dot_desired,  -1,1);
	  rc_saturate_float(&gamma_dot_desired,-1,1);
			
	  // use a small deadzone to prevent slow drifts in position
	  if(fabs(phi_dot_desired)<DSM_DEAD_ZONE)
	    phi_dot_desired  = 0.0;
	  if(fabs(gamma_dot_desired)<DSM_DEAD_ZONE)
	    gamma_dot_desired = 0.0;

	  // translate normalized user input to real setpoint values
	  switch(setpoint.drive_mode)
	    {
	    case NOVICE:
	      setpoint.phi_dot   = DRIVE_RATE_NOVICE * phi_dot_desired;
	      setpoint.gamma_dot = TURN_RATE_NOVICE  * gamma_dot_desired;
	      break;
	    case ADVANCED:
	      setpoint.phi_dot   = DRIVE_RATE_ADVANCED * phi_dot_desired;
	      setpoint.gamma_dot = TURN_RATE_ADVANCED  * gamma_dot_desired;
	      break;
	    default: break;
	    }
	}
      else
	// if we have NOT received recent commands, set the setpoint rates to zero
	{
	  setpoint.theta = 0;
	  setpoint.phi_dot = 0;
	  setpoint.gamma_dot = 0;
	  continue;
	}
    }		



  // if state becomes EXITING the above loop exists and we disarm here
  disarm_controller();
  pthread_exit(NULL);
  //	return NULL;
}

/*******************************************************************************
 * void balance_controller()
 *
 * discrete-time balance controller operated off IMU interrupt
 * Called at SAMPLE_RATE_HZ
 *******************************************************************************/
void balance_controller()
{
  static float cstate_pathlen = 0.0;
  static float cstate_phi_last = 0.0;
  float delta_phi;
  //  static int inner_saturation_counter = 0; 
  //  float dutyL, dutyR;
  /******************************************************************
   * STATE_ESTIMATION
   * read sensors and compute the state when either ARMED or DISARMED
   ******************************************************************/
  // angle theta is positive in the direction of forward tip around X axis
  cstate.theta = imu_data.dmp_TaitBryan[TB_PITCH_X] + CAPE_MOUNT_ANGLE; 
	
  // collect encoder positions, right wheel is reversed 
  cstate.wheelAngleR = (rc_get_encoder_pos(ENCODER_CHANNEL_R) * TWO_PI) 
    /(ENCODER_POLARITY_R * GEARBOX * ENCODER_RES);
  cstate.wheelAngleL = (rc_get_encoder_pos(ENCODER_CHANNEL_L) * TWO_PI) 
    /(ENCODER_POLARITY_L * GEARBOX * ENCODER_RES);
	
  // Phi is average wheel rotation also add theta body angle to get absolute 
  // wheel position in global frame since encoders are attachde to the body
  cstate.phi = ((cstate.wheelAngleL+cstate.wheelAngleR)/2.0) + cstate.theta;
  
  // steering angle gamma estimate 
  cstate.gamma = (cstate.wheelAngleR-cstate.wheelAngleL) 
    * (WHEEL_RADIUS_M/TRACK_WIDTH_M);

  // 2017-02-22 LLW Do some crude odometry
  // get mag heading  
  //body_frame_heading  = atan2(imu_data.mag[0],imu_data.mag[2]);
  body_frame_heading  = - cstate.gamma/2.0;

  // 2017-02-22 LLW compute distance traveled since last control cycle
  delta_phi = cstate.phi - cstate_phi_last;
  cstate_phi_last = cstate.phi;

  // 2017-02-22 LLW integrate position
  body_frame_easting  += sin(body_frame_heading) * delta_phi * WHEEL_RADIUS_M;
  body_frame_northing += cos(body_frame_heading) * delta_phi * WHEEL_RADIUS_M;
    
  //  body_frame_easting  += sin(cstate.gamma) * delta_phi * WHEEL_RADIUS_M;
  //  body_frame_northing += cos(cstate.gamma) * delta_phi * WHEEL_RADIUS_M;
  

  /*************************************************************
   * check for various exit conditions AFTER state estimate
   ***************************************************************/
  if(rc_get_state()==EXITING){
    rc_disable_motors();
    return;
  }
  // if controller is still ARMED while state is PAUSED, disarm it
  if(rc_get_state()!=RUNNING && setpoint.arm_state==ARMED){
    disarm_controller();
    return;
  }
  // exit if the controller is disarmed
  if(setpoint.arm_state==DISARMED){
    return;
  }
/*	
  // check for a tipover
  if(fabs(cstate.theta) > TIP_ANGLE){
    disarm_controller();
    ROS_INFO("tip detected \n");
    return;
  }
*/	
  /************************************************************
   * OUTER LOOP PHI controller D2
   * Move the position setpoint based on phi_dot. 
   * Input to the controller is phi error (setpoint-state).
   *************************************************************/
  /*if(ENABLE_POSITION_HOLD){
    if(setpoint.phi_dot != 0.0) setpoint.phi += setpoint.phi_dot*DT;
    cstate.d2_u = rc_march_filter(&D2,setpoint.phi-cstate.phi);
    setpoint.theta = cstate.d2_u;
  }
  else setpoint.theta = 0.0;
*/	
  /************************************************************
   * INNER LOOP ANGLE Theta controller D1
   * Input to D1 is theta error (setpoint-state). Then scale the 
   * output u to compensate for changing battery voltage.
   *************************************************************/
  //D1.gain = D1_GAIN * V_NOMINAL/cstate.vBatt;
  //cstate.d1_u = rc_march_filter(&D1,(setpoint.theta-cstate.theta));
	
  /*************************************************************
   * Check if the inner loop saturated. If it saturates for over
   * a second disarm the controller to prevent stalling motors.
   *************************************************************/
  /*if(fabs(cstate.d1_u)>0.95) inner_saturation_counter++;
  else inner_saturation_counter = 0; 
  // if saturate for a second, disarm for safety
  if(inner_saturation_counter > (SAMPLE_RATE_HZ*D1_SATURATION_TIMEOUT)){
    ROS_INFO("inner loop controller saturated\n");
    disarm_controller();
    inner_saturation_counter = 0;
    return;
  }
*/	
  /**********************************************************
   * gama (steering) controller D3
   * move the setpoint gamma based on user input like phi
   ***********************************************************/
/*  if(fabs(setpoint.gamma_dot)>0.0001)
    setpoint.gamma += setpoint.gamma_dot * DT;
  cstate.d3_u = rc_march_filter(&D3,setpoint.gamma - cstate.gamma);
*/	
  /**********************************************************
   * Send signal to motors
   * add D1 balance control u and D3 steering control also 
   * multiply by polarity to make sure direction is correct.
   ***********************************************************/
  //dutyL = cstate.d1_u - cstate.d3_u;
  //dutyR = cstate.d1_u + cstate.d3_u;	
  //ros::Subscriber sub = nh.subscribe("/visp_auto_tracker/object_position", 1, pose_callback);
  dutyL = DUTY_L_GLOBAL;
  dutyR = DUTY_R_GLOBAL;
 
  rc_set_motor(MOTOR_CHANNEL_L, MOTOR_POLARITY_L * dutyL); 
  rc_set_motor(MOTOR_CHANNEL_R, MOTOR_POLARITY_R * dutyR); 

  return;
}

/*******************************************************************************
 * 	zero_out_controller()
 *
 *	Clear the controller's memory and zero out setpoints.
 *******************************************************************************/
int zero_out_controller(){
  rc_reset_filter(&D1);
  rc_reset_filter(&D2);
  rc_reset_filter(&D3);
  setpoint.theta = 0.0f;
  setpoint.phi   = 0.0f;
  setpoint.gamma = 0.0f;
  rc_set_motor_all(0.0f);
  return 0;
}

/*******************************************************************************
 * disarm_controller()
 *
 * disable motors & set the setpoint.core_mode to DISARMED
 *******************************************************************************/
int disarm_controller(){
  rc_disable_motors();
  setpoint.arm_state = DISARMED;
  return 0;
}

/*******************************************************************************
 * arm_controller()
 *
 * zero out the controller & encoders. Enable motors & arm the controller.
 *******************************************************************************/
int arm_controller(){
  zero_out_controller();
  rc_set_encoder_pos(ENCODER_CHANNEL_L,0);
  rc_set_encoder_pos(ENCODER_CHANNEL_R,0);
  // prefill_filter_inputs(&D1,cstate.theta); 
  setpoint.arm_state = ARMED;
  rc_enable_motors();
  return 0;
}

/*******************************************************************************
 * int wait_for_starting_condition()
 *
 * Wait for MiP to be held upright long enough to begin.
 * Returns 0 if successful. Returns -1 if the wait process was interrupted by 
 * pause button or shutdown signal.
 *******************************************************************************/
int wait_for_starting_condition(){
  int checks = 0;
  const int check_hz = 20;	// check 20 times per second
  int checks_needed = round(START_DELAY*check_hz);
  int wait_us = 1000000/check_hz; 

  // wait for MiP to be tipped back or forward first
  // exit if state becomes paused or exiting
  while(rc_get_state()==RUNNING){
    // if within range, start counting
    if(fabs(cstate.theta) > START_ANGLE) checks++;
    // fell out of range, restart counter
    else checks = 0;
    // waited long enough, return
    if(checks >= checks_needed) break;
    rc_usleep(wait_us);
  }
  // now wait for MiP to be upright
  checks = 0;
  // exit if state becomes paused or exiting
  while(rc_get_state()==RUNNING){
    // if within range, start counting
    if(fabs(cstate.theta) < START_ANGLE) checks++;
    // fell out of range, restart counter
    else checks = 0;
    // waited long enough, return
    if(checks >= checks_needed) return 0;
    rc_usleep(wait_us);
  }
  return -1;
}

/*******************************************************************************
 * battery_checker()
 *
 * Slow loop checking battery voltage. Also changes the D1 saturation limit
 * since that is dependent on the battery voltage.
 *******************************************************************************/
void* battery_checker(void* ptr){
  float new_v;
  while(rc_get_state()!=EXITING){
    new_v = rc_battery_voltage();
    // if the value doesn't make sense, use nominal voltage
    if (new_v>9.0 || new_v<5.0) new_v = V_NOMINAL;
    cstate.vBatt = new_v;
    rc_usleep(1000000 / BATTERY_CHECK_HZ);
  }
  pthread_exit(NULL);	
  // return NULL;
}

/*******************************************************************************
 * printf_loop() 
 *
 * prints diagnostics to console
 * this only gets started if executing from terminal
 *******************************************************************************/
void* printf_loop(void* ptr)
{
  rc_state_t last_rc_state, new_rc_state; // keep track of last state
  edumip_msgs::EduMipState  edumip_state;

  new_rc_state = rc_get_state();
  
  while(rc_get_state()!=EXITING)
    {
      last_rc_state = new_rc_state; 
      new_rc_state = rc_get_state();


      edumip_state.setpoint_gamma_dot = setpoint.gamma_dot;
      edumip_state.setpoint_phi_dot   = setpoint.phi_dot;

      edumip_state.setpoint_phi       = setpoint.phi;
      edumip_state.phi                = cstate.phi;

      edumip_state.setpoint_gamma     = setpoint.gamma;
      edumip_state.gamma              = cstate.gamma;

      edumip_state.setpoint_theta     = setpoint.theta;
      edumip_state.theta              = cstate.theta;  

      edumip_state.d1_u               = cstate.d1_u;
      edumip_state.d3_u               = cstate.d3_u;

      edumip_state.dutyL              = dutyL;
      edumip_state.dutyR              = dutyR;

      edumip_state.wheel_angle_L      = cstate.wheelAngleL;
      edumip_state.wheel_angle_R      = cstate.wheelAngleR;

      edumip_state.body_frame_easting  = body_frame_easting;
      edumip_state.body_frame_northing = body_frame_northing;
      edumip_state.body_frame_heading  = body_frame_heading;
      
      edumip_state.vBatt              = cstate.vBatt;  
      edumip_state.armed              = (setpoint.arm_state == ARMED);
      edumip_state.running            = (new_rc_state == RUNNING);
      
      // publish the state
      state_publisher.publish(edumip_state);


      rc_usleep(1000000 / PRINTF_HZ);

    }

  pthread_exit(NULL);
  //return NULL;
} 

/*******************************************************************************
 * void on_pause_press() 
 *
 * Disarm the controller and set system state to paused.
 * If the user holds the pause button for 2 seconds, exit cleanly
 *******************************************************************************/
void on_pause_press(){
  int i=0;
  const int samples = 100;	// check for release 100 times in this period
  const int us_wait = 2000000; // 2 seconds
	
  switch(rc_get_state()){
    // pause if running
  case EXITING:
    return;
  case RUNNING:
    rc_set_state(PAUSED);
    disarm_controller();
    rc_set_led(RED,1);
    rc_set_led(GREEN,0);
    break;
  case PAUSED:
    rc_set_state(RUNNING);
    disarm_controller();
    rc_set_led(GREEN,1);
    rc_set_led(RED,0);
    break;
  default:
    break;
  }
	
  // now wait to see if the user want to shut down the program
  while(i<samples){
    rc_usleep(us_wait/samples);
    if(rc_get_pause_button() == RELEASED){
      return; //user let go before time-out
    }
    i++;
  }
  ROS_INFO("long press detected, shutting down\n");
  //user held the button down long enough, blink and exit cleanly
  rc_blink_led(RED,5,2);
  rc_set_state(EXITING);
  return;
}

/*******************************************************************************
 * void on_mode_release()
 *
 * toggle between position and angle modes if MiP is paused
 *******************************************************************************/
void on_mode_release()
{
  // toggle between position and angle modes
  if(setpoint.drive_mode == NOVICE){
    setpoint.drive_mode = ADVANCED;
    ROS_INFO("using drive_mode = ADVANCED\n");
  }
  else {
    setpoint.drive_mode = NOVICE;
    ROS_INFO("using drive_mode = NOVICE\n");
  }
	
  rc_blink_led(GREEN,5,1);
  return;
}


