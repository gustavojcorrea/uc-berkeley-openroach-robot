// Generated by gencpp from file visp_tracker/KltSettings.msg
// DO NOT EDIT!


#ifndef VISP_TRACKER_MESSAGE_KLTSETTINGS_H
#define VISP_TRACKER_MESSAGE_KLTSETTINGS_H


#include <string>
#include <vector>
#include <map>

#include <ros/types.h>
#include <ros/serialization.h>
#include <ros/builtin_message_traits.h>
#include <ros/message_operations.h>


namespace visp_tracker
{
template <class ContainerAllocator>
struct KltSettings_
{
  typedef KltSettings_<ContainerAllocator> Type;

  KltSettings_()
    : max_features(0)
    , window_size(0)
    , quality(0.0)
    , min_distance(0.0)
    , harris(0.0)
    , size_block(0)
    , pyramid_lvl(0)
    , mask_border(0)  {
    }
  KltSettings_(const ContainerAllocator& _alloc)
    : max_features(0)
    , window_size(0)
    , quality(0.0)
    , min_distance(0.0)
    , harris(0.0)
    , size_block(0)
    , pyramid_lvl(0)
    , mask_border(0)  {
  (void)_alloc;
    }



   typedef int64_t _max_features_type;
  _max_features_type max_features;

   typedef int64_t _window_size_type;
  _window_size_type window_size;

   typedef double _quality_type;
  _quality_type quality;

   typedef double _min_distance_type;
  _min_distance_type min_distance;

   typedef double _harris_type;
  _harris_type harris;

   typedef int64_t _size_block_type;
  _size_block_type size_block;

   typedef int64_t _pyramid_lvl_type;
  _pyramid_lvl_type pyramid_lvl;

   typedef int64_t _mask_border_type;
  _mask_border_type mask_border;





  typedef boost::shared_ptr< ::visp_tracker::KltSettings_<ContainerAllocator> > Ptr;
  typedef boost::shared_ptr< ::visp_tracker::KltSettings_<ContainerAllocator> const> ConstPtr;

}; // struct KltSettings_

typedef ::visp_tracker::KltSettings_<std::allocator<void> > KltSettings;

typedef boost::shared_ptr< ::visp_tracker::KltSettings > KltSettingsPtr;
typedef boost::shared_ptr< ::visp_tracker::KltSettings const> KltSettingsConstPtr;

// constants requiring out of line definition



template<typename ContainerAllocator>
std::ostream& operator<<(std::ostream& s, const ::visp_tracker::KltSettings_<ContainerAllocator> & v)
{
ros::message_operations::Printer< ::visp_tracker::KltSettings_<ContainerAllocator> >::stream(s, "", v);
return s;
}

} // namespace visp_tracker

namespace ros
{
namespace message_traits
{



// BOOLTRAITS {'IsFixedSize': True, 'IsMessage': True, 'HasHeader': False}
// {'std_msgs': ['/opt/ros/kinetic/share/std_msgs/cmake/../msg'], 'sensor_msgs': ['/opt/ros/kinetic/share/sensor_msgs/cmake/../msg'], 'geometry_msgs': ['/opt/ros/kinetic/share/geometry_msgs/cmake/../msg'], 'visp_tracker': ['/home/parallels/catkin_ws/src/vision_visp/visp_tracker/msg']}

// !!!!!!!!!!! ['__class__', '__delattr__', '__dict__', '__doc__', '__eq__', '__format__', '__getattribute__', '__hash__', '__init__', '__module__', '__ne__', '__new__', '__reduce__', '__reduce_ex__', '__repr__', '__setattr__', '__sizeof__', '__str__', '__subclasshook__', '__weakref__', '_parsed_fields', 'constants', 'fields', 'full_name', 'has_header', 'header_present', 'names', 'package', 'parsed_fields', 'short_name', 'text', 'types']




template <class ContainerAllocator>
struct IsFixedSize< ::visp_tracker::KltSettings_<ContainerAllocator> >
  : TrueType
  { };

template <class ContainerAllocator>
struct IsFixedSize< ::visp_tracker::KltSettings_<ContainerAllocator> const>
  : TrueType
  { };

template <class ContainerAllocator>
struct IsMessage< ::visp_tracker::KltSettings_<ContainerAllocator> >
  : TrueType
  { };

template <class ContainerAllocator>
struct IsMessage< ::visp_tracker::KltSettings_<ContainerAllocator> const>
  : TrueType
  { };

template <class ContainerAllocator>
struct HasHeader< ::visp_tracker::KltSettings_<ContainerAllocator> >
  : FalseType
  { };

template <class ContainerAllocator>
struct HasHeader< ::visp_tracker::KltSettings_<ContainerAllocator> const>
  : FalseType
  { };


template<class ContainerAllocator>
struct MD5Sum< ::visp_tracker::KltSettings_<ContainerAllocator> >
{
  static const char* value()
  {
    return "7cd8ae2f3a31d26015e8c80e88eb027a";
  }

  static const char* value(const ::visp_tracker::KltSettings_<ContainerAllocator>&) { return value(); }
  static const uint64_t static_value1 = 0x7cd8ae2f3a31d260ULL;
  static const uint64_t static_value2 = 0x15e8c80e88eb027aULL;
};

template<class ContainerAllocator>
struct DataType< ::visp_tracker::KltSettings_<ContainerAllocator> >
{
  static const char* value()
  {
    return "visp_tracker/KltSettings";
  }

  static const char* value(const ::visp_tracker::KltSettings_<ContainerAllocator>&) { return value(); }
};

template<class ContainerAllocator>
struct Definition< ::visp_tracker::KltSettings_<ContainerAllocator> >
{
  static const char* value()
  {
    return "# This message contains tracking parameters.\n\
#\n\
# These parameters determine how precise, how fast and how\n\
# reliable will be the tracking.\n\
#\n\
# It should be tuned carefully and can be changed dynamically.\n\
#\n\
# For more details, see the ViSP documentation:\n\
# http://www.irisa.fr/lagadic/visp/publication.html\n\
\n\
# Klt Parameters.\n\
\n\
int64 max_features      # Maximum number of features\n\
int64 window_size       # Window size\n\
float64 quality         # Quality of the tracker\n\
float64 min_distance      # Minimum distance betwenn two points\n\
float64 harris          # Harris free parameters\n\
int64 size_block        # Block size\n\
int64 pyramid_lvl       # Pyramid levels\n\
int64 mask_border       # Mask Border\n\
\n\
";
  }

  static const char* value(const ::visp_tracker::KltSettings_<ContainerAllocator>&) { return value(); }
};

} // namespace message_traits
} // namespace ros

namespace ros
{
namespace serialization
{

  template<class ContainerAllocator> struct Serializer< ::visp_tracker::KltSettings_<ContainerAllocator> >
  {
    template<typename Stream, typename T> inline static void allInOne(Stream& stream, T m)
    {
      stream.next(m.max_features);
      stream.next(m.window_size);
      stream.next(m.quality);
      stream.next(m.min_distance);
      stream.next(m.harris);
      stream.next(m.size_block);
      stream.next(m.pyramid_lvl);
      stream.next(m.mask_border);
    }

    ROS_DECLARE_ALLINONE_SERIALIZER
  }; // struct KltSettings_

} // namespace serialization
} // namespace ros

namespace ros
{
namespace message_operations
{

template<class ContainerAllocator>
struct Printer< ::visp_tracker::KltSettings_<ContainerAllocator> >
{
  template<typename Stream> static void stream(Stream& s, const std::string& indent, const ::visp_tracker::KltSettings_<ContainerAllocator>& v)
  {
    s << indent << "max_features: ";
    Printer<int64_t>::stream(s, indent + "  ", v.max_features);
    s << indent << "window_size: ";
    Printer<int64_t>::stream(s, indent + "  ", v.window_size);
    s << indent << "quality: ";
    Printer<double>::stream(s, indent + "  ", v.quality);
    s << indent << "min_distance: ";
    Printer<double>::stream(s, indent + "  ", v.min_distance);
    s << indent << "harris: ";
    Printer<double>::stream(s, indent + "  ", v.harris);
    s << indent << "size_block: ";
    Printer<int64_t>::stream(s, indent + "  ", v.size_block);
    s << indent << "pyramid_lvl: ";
    Printer<int64_t>::stream(s, indent + "  ", v.pyramid_lvl);
    s << indent << "mask_border: ";
    Printer<int64_t>::stream(s, indent + "  ", v.mask_border);
  }
};

} // namespace message_operations
} // namespace ros

#endif // VISP_TRACKER_MESSAGE_KLTSETTINGS_H
