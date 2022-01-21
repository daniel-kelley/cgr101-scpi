#
#  cgr101_pwm.rb
#
#  Copyright (c) 2022 by Daniel Kelley
#

#
# PWM Interfaces
#
# | SOURce:PULSe:DCYcle      | +
# | SOURce:PULSe:DCYcle?    | +
# | SOURce:PULSe:FREQuency   |
# | SOURce:PULSe:FREQuency? |

module CGR101PWM

  #
  # SOURce:PULSe:DCYcle/DCYcle?
  #
  def test_pwm_001
    # get a value
    self.class.hdl.send("SOURce:PULSe:DCYcle?")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Float(s) }
    assert_equal(1, v0.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # set it
    self.class.hdl.send("SOURce:PULSe:DCYcle " + v0[0].to_s)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again
    self.class.hdl.send("SOURce:PULSe:DCYcle?")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Float(s) }
    assert_equal(1, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 1st value
    assert_equal(v0, v1)
  end

  #
  # SOURce:PULSe:FREQuency/FREQuency?
  #
  def test_pwm_002
    # get a value
    self.class.hdl.send("SOURce:PULSe:FREQuency?")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Float(s) }
    assert_equal(1, v0.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # set it
    self.class.hdl.send("SOURce:PULSe:FREQuency " + v0[0].to_s)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again
    self.class.hdl.send("SOURce:PULSe:FREQuency?")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Float(s) }
    assert_equal(1, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 1st value
    assert_equal(v0, v1)
  end

end
