#
#  cgr101_meas.rb
#
#  Copyright (c) 2022 by Daniel Kelley
#

#
# High level MEASURE and low level equivalents
#
module CGR101Meas

  #
  # MEASure:DIGITAL:DATA? - prescribed range and set of channel
  #
  def test_meas_001
    self.class.hdl.send("MEAS:DIG:DATA?")
    out = self.class.hdl.recv
    v = out.split(',').map { |s| Integer(s) }
    assert_equal(1, v.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # READ:DIGITAL:DATA? - prescribed range and set of channel
  #
  def test_meas_002
    # READ sequence
    self.class.hdl.send("ABORt")
    self.class.hdl.send("CONFIGURE:DIG:DATA")
    self.class.hdl.send("READ:DIG:DATA?")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Integer(s) }
    assert_equal(1, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # measure for reference
    self.class.hdl.send("MEAS:DIG:DATA?")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Integer(s) }

    assert_equal(v0, v1)
  end

  #
  # FETCh:DIGITAL:DATA? - prescribed range and set of channel
  #
  def test_meas_003
    # FETCH sequence
    self.class.hdl.send("ABORt")
    self.class.hdl.send("CONFIGURE:DIG:DATA")
    self.class.hdl.send("INITiate")
    self.class.hdl.send("FETCH:DIG:DATA?")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Integer(s) }
    assert_equal(1, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # measure for reference
    self.class.hdl.send("MEAS:DIG:DATA?")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Integer(s) }

    assert_equal(v0, v1)
  end

  #
  # SOURce:DIGital:DATA
  #
  def test_meas_004
    self.class.hdl.send("SOUR:DIG:DATA 1")
    self.class.hdl.send("SOUR:DIG:DATA?")
    out = self.class.hdl.recv
    v = out.split(',').map { |s| Integer(s) }
    assert_equal(1, v.length)
    assert_equal(1, v[0])
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

end
