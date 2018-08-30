# This Python file uses the following encoding: utf-8
"""autogenerated by genpy from visp_tracker/MovingEdgeSite.msg. Do not edit."""
import sys
python3 = True if sys.hexversion > 0x03000000 else False
import genpy
import struct


class MovingEdgeSite(genpy.Message):
  _md5sum = "d67448def98304944978d0ca12803af8"
  _type = "visp_tracker/MovingEdgeSite"
  _has_header = False #flag to mark the presence of a Header object
  _full_text = """# Moving edge position.

float64 x      # X position in the image
float64 y      # Y position in the image
int32 suppress # Is the moving edge valid?
               # - 0:   yes
	       # - 1:   no, constrast check has failed.
	       # - 2:   no, threshold check has failed.
	       # - 3:   no, M-estimator check has failed.
	       # - >=4: no, undocumented reason.
"""
  __slots__ = ['x','y','suppress']
  _slot_types = ['float64','float64','int32']

  def __init__(self, *args, **kwds):
    """
    Constructor. Any message fields that are implicitly/explicitly
    set to None will be assigned a default value. The recommend
    use is keyword arguments as this is more robust to future message
    changes.  You cannot mix in-order arguments and keyword arguments.

    The available fields are:
       x,y,suppress

    :param args: complete set of field values, in .msg order
    :param kwds: use keyword arguments corresponding to message field names
    to set specific fields.
    """
    if args or kwds:
      super(MovingEdgeSite, self).__init__(*args, **kwds)
      #message fields cannot be None, assign default values for those that are
      if self.x is None:
        self.x = 0.
      if self.y is None:
        self.y = 0.
      if self.suppress is None:
        self.suppress = 0
    else:
      self.x = 0.
      self.y = 0.
      self.suppress = 0

  def _get_types(self):
    """
    internal API method
    """
    return self._slot_types

  def serialize(self, buff):
    """
    serialize message into buffer
    :param buff: buffer, ``StringIO``
    """
    try:
      _x = self
      buff.write(_get_struct_2di().pack(_x.x, _x.y, _x.suppress))
    except struct.error as se: self._check_types(struct.error("%s: '%s' when writing '%s'" % (type(se), str(se), str(locals().get('_x', self)))))
    except TypeError as te: self._check_types(ValueError("%s: '%s' when writing '%s'" % (type(te), str(te), str(locals().get('_x', self)))))

  def deserialize(self, str):
    """
    unpack serialized message in str into this message instance
    :param str: byte array of serialized message, ``str``
    """
    try:
      end = 0
      _x = self
      start = end
      end += 20
      (_x.x, _x.y, _x.suppress,) = _get_struct_2di().unpack(str[start:end])
      return self
    except struct.error as e:
      raise genpy.DeserializationError(e) #most likely buffer underfill


  def serialize_numpy(self, buff, numpy):
    """
    serialize message with numpy array types into buffer
    :param buff: buffer, ``StringIO``
    :param numpy: numpy python module
    """
    try:
      _x = self
      buff.write(_get_struct_2di().pack(_x.x, _x.y, _x.suppress))
    except struct.error as se: self._check_types(struct.error("%s: '%s' when writing '%s'" % (type(se), str(se), str(locals().get('_x', self)))))
    except TypeError as te: self._check_types(ValueError("%s: '%s' when writing '%s'" % (type(te), str(te), str(locals().get('_x', self)))))

  def deserialize_numpy(self, str, numpy):
    """
    unpack serialized message in str into this message instance using numpy for array types
    :param str: byte array of serialized message, ``str``
    :param numpy: numpy python module
    """
    try:
      end = 0
      _x = self
      start = end
      end += 20
      (_x.x, _x.y, _x.suppress,) = _get_struct_2di().unpack(str[start:end])
      return self
    except struct.error as e:
      raise genpy.DeserializationError(e) #most likely buffer underfill

_struct_I = genpy.struct_I
def _get_struct_I():
    global _struct_I
    return _struct_I
_struct_2di = None
def _get_struct_2di():
    global _struct_2di
    if _struct_2di is None:
        _struct_2di = struct.Struct("<2di")
    return _struct_2di
