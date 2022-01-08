#
#  cgr101_scope.rb
#
#  Copyright (c) 2022 by Daniel Kelley
#

#
# Oscilloscope interfaces
#
# | INP:COUP coupling_arg                   | Not Implemented
# | SENS:SWE:POIN?                          | +
# | SENS:SWE:OFFS:POIN numeric_value        | +
# | SENS:SWE:OFFS:POIN?                     | +
# | SENS:SWE:OFFS:TIME numeric_value        |
# | SENS:SWE:OFFS:TIME?                     |
# | SENS:SWE:OREF:LOC numeric_value         |
# | SENS:SWE:OREF:LOC?                      |
# | SENS:SWE:OREF:POIN numeric_value        |
# | SENS:SWE:OREF:POIN?                     |
# | SENS:SWE:TIME numeric_value             |
# | SENS:SWE:TIME? numeric_value            | needs implementation
# | SENS:SWE:TINT numeric_value             |
# | SENS:SWE:TINT? numeric_value            | needs implementation
# | SENS:VOLT:DC:LOW numeric_value channel  |
# | SENS:VOLT:DC:LOW? channel               |
# | SENS:VOLT:DC:OFFS numeric_value channel |
# | SENS:VOLT:DC:OFFS? channel              |
# | SENS:VOLT:DC:PTP numeric_value channel  |
# | SENS:VOLT:DC:PTP? channel               |
# | SENS:VOLT:DC:RANG numeric_value channel |
# | SENS:VOLT:DC:RANG? channel              |
# | SENS:VOLT:DC:UPP numeric_value channel  |
# | SENS:VOLT:DC:UPP? channel               |
# | SENS:VOLT:LOW numeric_value channel     |
# | SENS:VOLT:LOW? channel                  |
# | SENS:VOLT:OFFS numeric_value channel    |
# | SENS:VOLT:OFFS?  channel                |
# | SENS:VOLT:PTP numeric_value channel     |
# | SENS:VOLT:PTP? channel                  |
# | SENS:VOLT:RANG numeric_value channel    |
# | SENS:VOLT:RANG? channel                 |
# | SENS:VOLT:UPP numeric_value channel     |
# | SENS:VOLT:UPP? channel                  |
#
# | SENS:DAT? channel                       |
# | SENS:FUNC:CONC boolean                  | Not Implemented
# | SENS:FUNC:OFF channel                   |
# | SENS:FUNC:ON channel                    |
# | SENS:FUNC:STAT? channel                 |
# | SENS:RES                                |

module CGR101Scope

  #
  # SENS:SWE:POIN?
  #
  def test_scope_001
    self.class.hdl.send("SENS:SWE:POIN?")
    out = self.class.hdl.recv
    v = out.split(',').map { |s| Integer(s) }
    assert_equal(1, v.length)
    assert_equal(1024, v[0])
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # no error reports
    self.class.hdl.send("SYSTem:ERRor:COUNt?")
    out = self.class.hdl.recv
    assert_equal("0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SENS:SWE:OFFS:POIN/POIN?
  #
  def test_scope_002
    # get a value
    self.class.hdl.send("SENS:SWE:OFFS:POIN?")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Integer(s) }
    assert_equal(1, v0.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # set it
    self.class.hdl.send("SENS:SWE:OFFS:POIN " + v0[0].to_s)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again
    self.class.hdl.send("SENS:SWE:OFFS:POIN?")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Integer(s) }
    assert_equal(1, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 1st value
    assert_equal(v0, v1)
  end

  #
  # SENS:SWE:OFFS:TIME/TIME?
  #
  def no_test_scope_003
    # get a value
    self.class.hdl.send("SENS:SWE:OFFS:TIME?")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Integer(s) }
    assert_equal(1, v0.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # set it
    self.class.hdl.send("SENS:SWE:OFFS:TIME " + v0[0].to_s)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again
    self.class.hdl.send("SENS:SWE:OFFS:TIME?")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Integer(s) }
    assert_equal(1, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 1st value
    assert_equal(v0, v1)
  end

end
