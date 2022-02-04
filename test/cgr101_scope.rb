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
# | SENS:SWE:OFFS:TIME numeric_value        | +
# | SENS:SWE:OFFS:TIME?                     | +
# | SENS:SWE:OREF:LOC numeric_value         | +
# | SENS:SWE:OREF:LOC?                      | +
# | SENS:SWE:OREF:POIN numeric_value        | +
# | SENS:SWE:OREF:POIN?                     | +
# | SENS:SWE:TIME numeric_value             | +
# | SENS:SWE:TIME? numeric_value            | +
# | SENS:SWE:TINT numeric_value             | +
# | SENS:SWE:TINT? numeric_value            | +
# | SENS:VOLT:DC:LOW numeric_value channel  | +
# | SENS:VOLT:DC:LOW? channel               | +
# | SENS:VOLT:DC:OFFS numeric_value channel | +
# | SENS:VOLT:DC:OFFS? channel              | +
# | SENS:VOLT:DC:PTP numeric_value channel  | +
# | SENS:VOLT:DC:PTP? channel               | +
# | SENS:VOLT:DC:RANG numeric_value channel | +
# | SENS:VOLT:DC:RANG? channel              | +
# | SENS:VOLT:DC:UPP numeric_value channel  | +
# | SENS:VOLT:DC:UPP? channel               | +
# | SENS:VOLT:LOW numeric_value channel     | +
# | SENS:VOLT:LOW? channel                  | +
# | SENS:VOLT:OFFS numeric_value channel    | +
# | SENS:VOLT:OFFS?  channel                | +
# | SENS:VOLT:PTP numeric_value channel     | +
# | SENS:VOLT:PTP? channel                  | +
# | SENS:VOLT:RANG numeric_value channel    | +
# | SENS:VOLT:RANG? channel                 | +
# | SENS:VOLT:UPP numeric_value channel     | +
# | SENS:VOLT:UPP? channel                  | +
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
  def test_scope_003
    # get a value
    self.class.hdl.send("SENS:SWE:OFFS:TIME?")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Float(s) }
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
    v1 = out.split(',').map { |s| Float(s) }
    assert_equal(1, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 1st value
    assert_equal(v0, v1)
  end

  #
  # SENS:SWE:OREF:LOC/LOC?
  #
  def test_scope_004
    # get a value
    self.class.hdl.send("SENS:SWE:OREF:LOC?")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Float(s) }
    assert_equal(1, v0.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # set it
    self.class.hdl.send("SENS:SWE:OREF:LOC " + v0[0].to_s)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again
    self.class.hdl.send("SENS:SWE:OREF:LOC?")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Float(s) }
    assert_equal(1, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 1st value
    assert_equal(v0, v1)
  end

  #
  # SENS:SWE:OREF:POIN/POIN?
  #
  def test_scope_005
    # get a value
    self.class.hdl.send("SENS:SWE:OREF:POIN?")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Integer(s) }
    assert_equal(1, v0.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # set it
    self.class.hdl.send("SENS:SWE:OREF:POIN " + v0[0].to_s)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again
    self.class.hdl.send("SENS:SWE:OREF:POIN?")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Integer(s) }
    assert_equal(1, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 1st value
    assert_equal(v0, v1)
  end

  #
  # SENS:SWE:TIME/TIME?
  #
  def test_scope_006
    # get a value
    self.class.hdl.send("SENS:SWE:TIME?")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Float(s) }
    assert_equal(1, v0.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
    # default should be some positive value
    assert(v0[0] > 0.0)

    # set it
    self.class.hdl.send("SENS:SWE:TIME " + v0[0].to_s)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again
    self.class.hdl.send("SENS:SWE:TIME?")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Float(s) }
    assert_equal(1, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 1st value
    assert_equal(v0, v1)
  end

  #
  # SENS:SWE:TINT/TINT?
  #
  def test_scope_007
    # get a value
    self.class.hdl.send("SENS:SWE:TINT?")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Float(s) }
    assert_equal(1, v0.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
    # default should be some positive value
    assert(v0[0] > 0.0)

    # set it
    self.class.hdl.send("SENS:SWE:TINT " + v0[0].to_s)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again
    self.class.hdl.send("SENS:SWE:TINT?")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Float(s) }
    assert_equal(1, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 1st value
    assert_equal(v0, v1)
  end

  #
  # SENS:VOLT[:DC]:LOW/LOW? + multiple channels
  #
  def test_scope_008
    # get a value (long form)
    self.class.hdl.send("SENS:VOLT:DC:LOW? (@1,2)")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Float(s) }
    assert_equal(2, v0.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # set it (long form)
    self.class.hdl.send("SENS:VOLT:DC:LOW " + v0[0].to_s + " (@1,2)")
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again (short form)
    self.class.hdl.send("SENS:VOLT:LOW? (@1,2)")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Float(s) }
    assert_equal(2, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 1st value
    assert_equal(v0, v1)

    # set it again (short form)
    self.class.hdl.send("SENS:VOLT:LOW " + v0[0].to_s + " (@1,2)")
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again (short form)
    self.class.hdl.send("SENS:VOLT:LOW? (@1,2)")
    out = self.class.hdl.recv
    v2 = out.split(',').map { |s| Float(s) }
    # should match 2nd value
    assert_equal(v1, v2)

  end

  #
  # SENS:VOLT[:DC]:OFFS/OFFS? + multiple channels
  #
  def test_scope_009
    # get a value (long form)
    self.class.hdl.send("SENS:VOLT:DC:OFFS? (@1,2)")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Float(s) }
    assert_equal(2, v0.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # set it (long form)
    self.class.hdl.send("SENS:VOLT:DC:OFFS " + v0[0].to_s + " (@1,2)")
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again (short form)
    self.class.hdl.send("SENS:VOLT:OFFS? (@1,2)")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Float(s) }
    assert_equal(2, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 1st value
    assert_equal(v0, v1)

    # set it again (short form)
    self.class.hdl.send("SENS:VOLT:OFFS " + v0[0].to_s + " (@1,2)")
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again (short form)
    self.class.hdl.send("SENS:VOLT:OFFS? (@1,2)")
    out = self.class.hdl.recv
    v2 = out.split(',').map { |s| Float(s) }
    # should match 2nd value
    assert_equal(v1, v2)

  end

  #
  # SENS:VOLT[:DC]:PTP/PTP? + multiple channels
  #
  def test_scope_010
    # get a value (long form)
    self.class.hdl.send("SENS:VOLT:DC:PTP? (@1,2)")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Float(s) }
    assert_equal(2, v0.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # set it (long form)
    self.class.hdl.send("SENS:VOLT:DC:PTP " + v0[0].to_s + " (@1,2)")
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again (short form)
    self.class.hdl.send("SENS:VOLT:PTP? (@1,2)")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Float(s) }
    assert_equal(2, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 1st value
    assert_equal(v0, v1)

    # set it again (short form)
    self.class.hdl.send("SENS:VOLT:PTP " + v0[0].to_s + " (@1,2)")
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again (short form)
    self.class.hdl.send("SENS:VOLT:PTP? (@1,2)")
    out = self.class.hdl.recv
    v2 = out.split(',').map { |s| Float(s) }
    # should match 2nd value
    assert_equal(v1, v2)

  end

  #
  # SENS:VOLT[:DC]:RANG/RANG? + multiple channels
  #
  def test_scope_011
    # get a value (long form)
    self.class.hdl.send("SENS:VOLT:DC:RANG? (@1,2)")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Float(s) }
    assert_equal(2, v0.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # set it (long form)
    self.class.hdl.send("SENS:VOLT:DC:RANG " + v0[0].to_s + " (@1,2)")
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again (short form)
    self.class.hdl.send("SENS:VOLT:RANG? (@1,2)")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Float(s) }
    assert_equal(2, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 1st value
    assert_equal(v0, v1)

    # set it again (short form)
    self.class.hdl.send("SENS:VOLT:RANG " + v0[0].to_s + " (@1,2)")
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again (short form)
    self.class.hdl.send("SENS:VOLT:RANG? (@1,2)")
    out = self.class.hdl.recv
    v2 = out.split(',').map { |s| Float(s) }
    # should match 2nd value
    assert_equal(v1, v2)

  end

  #
  # SENS:VOLT[:DC]:UPP/UPP? + multiple channels
  #
  def test_scope_012
    # get a value (long form)
    self.class.hdl.send("SENS:VOLT:DC:UPP? (@1,2)")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Float(s) }
    assert_equal(2, v0.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # set it (long form)
    self.class.hdl.send("SENS:VOLT:DC:UPP " + v0[0].to_s + " (@1,2)")
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again (short form)
    self.class.hdl.send("SENS:VOLT:UPP? (@1,2)")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Float(s) }
    assert_equal(2, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # should match 1st value
    assert_equal(v0, v1)

    # set it again (short form)
    self.class.hdl.send("SENS:VOLT:UPP " + v0[0].to_s + " (@1,2)")
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # get it again (short form)
    self.class.hdl.send("SENS:VOLT:UPP? (@1,2)")
    out = self.class.hdl.recv
    v2 = out.split(',').map { |s| Float(s) }
    # should match 2nd value
    assert_equal(v1, v2)

  end

  #
  # Manual Trigger Smoke Test
  #
  def test_scope_data_imm
    self.class.hdl.send("SENS:SWE:POIN?")
    out = self.class.hdl.recv
    points = Integer(out)
    self.class.hdl.send("SENS:FUNC:ON (@1)")
    self.class.hdl.send("INIT:IMM")

    # Use STAT:OPER:COND? bit 3 (SWEEP status) and poll for completion
    status = 0
    20.times do
      self.class.hdl.send("STAT:OPER:COND?")
      out = self.class.hdl.recv
      status = Integer(out)
      if (status & 8) == 0
        break
      end
      sleep(0.1)
    end
    assert_equal(status & 8, 0)

    self.class.hdl.send("SENS:DATA? (@1)")
    out = self.class.hdl.recv
    v1 = out.split(',') #.map { |s| Float(s) }
    assert_equal(points, v1.length)
  end

  #
  # Manual Trigger + blocking DATA?
  #
  def test_scope_data_block
    self.class.hdl.send("SENS:SWE:POIN?")
    out = self.class.hdl.recv
    points = Integer(out)
    self.class.hdl.send("SENS:FUNC:ON (@1)")
    self.class.hdl.send("INIT:IMM")

    self.class.hdl.send("SENS:DATA? (@1)")
    out = self.class.hdl.recv
    v1 = out.split(',') #.map { |s| Float(s) }
    assert_equal(points, v1.length)
  end

  #
  # Manual Trigger + *WAI
  #
  def test_scope_data_wai
    self.class.hdl.send("SENS:SWE:POIN?")
    out = self.class.hdl.recv
    points = Integer(out)
    self.class.hdl.send("SENS:FUNC:ON (@1)")
    self.class.hdl.send("INIT:IMM")
    self.class.hdl.send("*WAI")
    self.class.hdl.send("SENS:DATA? (@1)")
    out = self.class.hdl.recv
    v1 = out.split(',') #.map { |s| Float(s) }
    assert_equal(points, v1.length)
  end

  #
  # Manual Trigger + *OPC?
  #
  def test_scope_data_opcq
    self.class.hdl.send("SENS:SWE:POIN?")
    out = self.class.hdl.recv
    points = Integer(out)
    self.class.hdl.send("SENS:FUNC:ON (@1)")
    self.class.hdl.send("INIT:IMM")
    self.class.hdl.send("*OPC?")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Integer(s) }
    assert_equal(1, v0.length)
    assert_equal(1, v0[0])

    self.class.hdl.send("SENS:DATA? (@1)")
    out = self.class.hdl.recv
    v1 = out.split(',') #.map { |s| Float(s) }
    assert_equal(points, v1.length)
  end

  #
  # Abort Test
  #
  def test_scope_data_abort
    self.class.hdl.send("SENS:FUNC:ON (@1)")
    self.class.hdl.send("INIT:IMM")
    # Use STAT:OPER:COND? bit 3 (SWEEP status)
    self.class.hdl.send("STAT:OPER:COND?")
    out = self.class.hdl.recv
    # Send abort ASAP
    self.class.hdl.send("ABORT")
    status = Integer(out)
    # status should have indicated sweep
    assert_equal(status & 8, 8)
    # after ABORt, status should be 0
    self.class.hdl.send("STAT:OPER:COND?")
    out = self.class.hdl.recv
    status = Integer(out)
    assert_equal(status & 8, 0)
    # May or may not have data so don't count on it.
  end

end
