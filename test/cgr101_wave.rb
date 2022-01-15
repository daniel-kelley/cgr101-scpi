#
#  cgr101_wave.rb
#
#  Copyright (c) 2022 by Daniel Kelley
#

#
# Waveform interfaces
#
# | SOUR:FREQ numeric_value                 | +
# | SOUR:FREQ?                              | +
# | SOUR:FUNC:LEV numeric_value             | +
# | SOUR:FUNC:LEV?                          | +
# | SOUR:FUNC source_function               | WIP
# | SOUR:FUNC?                              | WIP
# | SOUR:FUNC:USER block                    |
# | SOUR:FUNC:USER?                         |

module CGR101Wave

  #
  # SOUR:FREQ/FREQ?
  #
  def test_wave_001
    # get a value
    self.class.hdl.send("SOUR:FREQ?")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Integer(s) }
    assert_equal(1, v0.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # set it
    self.class.hdl.send("SOUR:FREQ " + v0[0].to_s)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again
    self.class.hdl.send("SOUR:FREQ?")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Integer(s) }
    assert_equal(1, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 1st value
    assert_equal(v0, v1)
  end

  #
  # SOUR:FUNC:LEV/LEV?
  #
  def test_wave_002
    # get a value
    self.class.hdl.send("SOUR:FUNC:LEV?")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Integer(s) }
    assert_equal(1, v0.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # set it
    self.class.hdl.send("SOUR:FUNC:LEV " + v0[0].to_s)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again
    self.class.hdl.send("SOUR:FUNC:LEV?")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Integer(s) }
    assert_equal(1, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 1st value
    assert_equal(v0, v1)
  end

  #
  # SOUR:FUNC/FUNC?
  #
  def test_wave_003a
    # get a value
    self.class.hdl.send("SOUR:FUNC?")
    out = self.class.hdl.recv
    v0 = out.split(',')
    assert_equal(1, v0.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # set it
    self.class.hdl.send("SOUR:FUNC " + v0[0])
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again
    self.class.hdl.send("SOUR:FUNC?")
    out = self.class.hdl.recv
    v1 = out.split(',')
    assert_equal(1, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 1st value
    assert_equal(v0, v1)
  end

  #
  # SOUR:FUNC/FUNC? {SIN,SQU,TRI,RAND}
  #
  def test_wave_003b
    ["SIN","SQU","TRI","RAND"].each do |func|
      # set it
      self.class.hdl.send("SOUR:FUNC " + func)
      assert_equal(0, self.class.hdl.out_length)
      assert_equal(0, self.class.hdl.err_length)

      # get it again
      self.class.hdl.send("SOUR:FUNC?")
      out = self.class.hdl.recv
      v1 = out.split(',')
      assert_equal(1, v1.length)
      assert_equal(0, self.class.hdl.out_length)
      assert_equal(0, self.class.hdl.err_length)

      # should match 1st value
      assert_equal(func, v1[0])
    end
  end

  #
  # SOUR:FUNC:USER/USER?
  #
  def test_wave_004
    # set it
    self.class.hdl.send("SOUR:FUNC:USER -1.0,1.0")
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it
    self.class.hdl.send("SOUR:FUNC:USER?")
    out = self.class.hdl.recv
    v0 = out.split(',')
    assert_equal(256, v0.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get SOUR:FUNC?
    self.class.hdl.send("SOUR:FUNC?")
    out = self.class.hdl.recv
    v1 = out.split(',')
    assert_equal(1, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 'USER'
    assert_equal('USER', v1[0])

  end

end
