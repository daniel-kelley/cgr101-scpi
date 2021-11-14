#
#  cgr101_testcase.rb
#
#  Copyright (c) 2021 by Daniel Kelley
#

require 'pp'

#
# Command test cases
#
module CGR101Testcase

  #
  # Interface handle check
  #
  def test_001
    hdl = self.class.hdl
    assert_not_nil hdl
    assert hdl.wthr.alive?
  end

  #
  # Device Setup completed correctly
  #
  def no_test_002
    self.class.hdl.send("SYSTem:INTernal:SETUP?")
    out = self.class.hdl.recv
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard SYST:VERS?
  #
  def test_003
    self.class.hdl.send("SYST:VERS?")
    out = self.class.hdl.recv
    assert_equal("1999.0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard SYSTem:ERRor:COUNt?
  #
  def test_004
    self.class.hdl.send("SYSTem:ERRor:COUNt?")
    out = self.class.hdl.recv
    assert_equal("0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard SYSTem:ERRor?
  #
  def test_005
    self.class.hdl.send("SYSTem:ERRor?")
    out = self.class.hdl.recv
    assert_equal("0,\"No error\"", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # "No error" command behavior
  #
  def test_006
    self.class.hdl.send("SYSTem:ERRor:NEXT?")
    out = self.class.hdl.recv
    assert_equal("0,\"No error\"", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # "Has error" command behavior
  #
  def no_test_007
    # bogus command
    self.class.hdl.send("SYSTem:BOGUS")
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # error response should reflect bogus command
    self.class.hdl.send("SYSTem:ERRor:NEXT?")
    out = self.class.hdl.recv
    assert_equal("-113,\"Undefined header;SYSTem:BOGUS\"", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
    # no more errors
    self.class.hdl.send("SYSTem:ERRor:NEXT?")
    out = self.class.hdl.recv
    assert_equal("0,\"No error\"", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard SYSTem:CAPability?
  #
  def no_test_008
    self.class.hdl.send("SYSTem:CAPability?")
    out = self.class.hdl.recv
    assert_equal("\"DCVOLTMETER\"", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard *CLS
  #
  def no_test_009
    # create error with bogus command
    self.class.hdl.send("SYSTem:BOGUS")
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    # Clear errors
    self.class.hdl.send("*CLS")

    # no more errors
    self.class.hdl.send("SYSTem:ERRor:NEXT?")
    out = self.class.hdl.recv
    assert_equal("0,\"No error\"", out)
  end

  #
  # SCPI standard *ESE?, *ESE
  #
  def test_010
    self.class.hdl.send("*ese?")
    ese = self.class.hdl.recv
    assert_not_nil(ese)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    self.class.hdl.send("*ese #{ese}")

    self.class.hdl.send("*ese?")
    out = self.class.hdl.recv
    assert_not_nil(out)
    assert_equal(ese, out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard *ESR?
  #
  def no_test_011
    self.class.hdl.send("*ESR?")
    out = self.class.hdl.recv
    assert_not_nil(out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard *IDN?
  #
  def no_test_012
    self.class.hdl.send("*idn?")
    out = self.class.hdl.recv
    assert_equal("GMP,CGR101-SCPI,1.0,01-02", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard *OPC, *OPC?, *ESR clearing
  #
  def no_test_013
    self.class.hdl.send("*opc")
    self.class.hdl.send("*opc?")
    out = self.class.hdl.recv
    assert_equal("1", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    self.class.hdl.send("*esr?")
    out = self.class.hdl.recv
    status_byte = Integer(out)
    assert_equal(1, status_byte & 1)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    self.class.hdl.send("*esr?")
    out = self.class.hdl.recv
    status_byte = Integer(out)
    assert_equal(0, status_byte & 1)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard *RST
  #
  def no_test_014
    self.class.hdl.send("*rst")
    self.class.hdl.send("*opc?")
    out = self.class.hdl.recv
    assert_equal("1", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard *SRE?, *SRE
  #
  def no_test_015
    self.class.hdl.send("*sre?")
    sre = self.class.hdl.recv
    assert_not_nil(sre)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    self.class.hdl.send("*sre #{sre}")

    self.class.hdl.send("*sre?")
    out = self.class.hdl.recv
    assert_not_nil(out)
    assert_equal(sre, out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard *STB?
  #
  def no_test_014
    self.class.hdl.send("*stb?")
    out = self.class.hdl.recv
    assert_equal("4", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard *TST?
  #
  def no_test_015
    self.class.hdl.send("*tst?")
    out = self.class.hdl.recv
    assert_equal("0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard *WAI
  #
  def no_test_014
    self.class.hdl.send("*WAI")
    self.class.hdl.send("*opc?")
    out = self.class.hdl.recv
    assert_equal("1", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard STATus:OPERation?
  #
  def no_test_015
    self.class.hdl.send("STATus:OPERation?")
    out = self.class.hdl.recv
    assert_equal("0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard STATus:OPERation:EVENt?
  #
  def no_test_016
    self.class.hdl.send("STATus:OPERation:EVENt?")
    out = self.class.hdl.recv
    assert_equal("0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard STATus:OPERation:CONDition?
  #
  def no_test_017
    self.class.hdl.send("STATus:OPERation:CONDition?")
    out = self.class.hdl.recv
    assert_equal("0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard STAT:OPER:ENAB?, STAT:OPER:ENAB
  #
  def no_test_018
    self.class.hdl.send("STAT:OPER:ENAB?")
    soe = self.class.hdl.recv
    assert_not_nil(soe)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    self.class.hdl.send("STAT:OPER:ENAB #{soe}")

    self.class.hdl.send("STAT:OPER:ENAB?")
    out = self.class.hdl.recv
    assert_not_nil(out)
    assert_equal(soe, out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard STATus:QUEStionable?
  #
  def no_test_019
    self.class.hdl.send("STATus:QUEStionable?")
    out = self.class.hdl.recv
    assert_equal("0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard STAT:OPER:ENAB?, STAT:OPER:ENAB
  #
  def no_test_020
    self.class.hdl.send("STAT:QUES:ENAB?")
    sqe = self.class.hdl.recv
    assert_not_nil(sqe)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    self.class.hdl.send("STAT:QUES:ENAB #{sqe}")

    self.class.hdl.send("STAT:QUES:ENAB?")
    out = self.class.hdl.recv
    assert_not_nil(out)
    assert_equal(sqe, out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard STATus:PRESet
  #
  def no_test_021
    self.class.hdl.send("STATus:PRESet")
    self.class.hdl.send("*opc?")
    out = self.class.hdl.recv
    assert_equal("1", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # U3 MEASure:DIGITAL:DATA? - prescribed range and set of channel
  #
  def no_test_029
    self.class.hdl.send("MEAS:DIG:DATA? (@17:19)")
    out = self.class.hdl.recv
    v = out.split(',').map { |s| Integer(s) }
    assert_equal(3, v.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # U3 READ:DIGITAL:DATA? - prescribed range and set of channel
  #
  def no_test_030
    # measure for reference
    self.class.hdl.send("MEAS:DIG:DATA? (@17:19)")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Integer(s) }

    # READ sequence
    self.class.hdl.send("ABORt")
    self.class.hdl.send("CONFIGURE:DIG:DATA (@17:19)")
    self.class.hdl.send("READ:DIG:DATA? (@17:19)")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Integer(s) }
    assert_equal(3, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    assert_equal(v0, v1)
  end

  #
  # U3 FETCh:DIGITAL:DATA? - prescribed range and set of channel
  #
  def no_test_031
    # measure for reference
    self.class.hdl.send("MEAS:DIG:DATA? (@17:19)")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Integer(s) }

    # FETCH sequence
    self.class.hdl.send("ABORt")
    self.class.hdl.send("CONFIGURE:DIG:DATA (@17:19)")
    self.class.hdl.send("INITiate")
    self.class.hdl.send("FETCH:DIG:DATA? (@17:19)")
    out = self.class.hdl.recv
    v1 = out.split(',').map { |s| Integer(s) }
    assert_equal(3, v1.length)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)

    assert_equal(v0, v1)
  end

  def no_test_outline
    self.class.hdl.send("SYSTem:CAPability?")
    sleep(5)
    puts self.class.hdl.get_out
    puts self.class.hdl.get_err
    #out = self.class.hdl.recv
    #assert_equal("0,\"No error\"", out)
    #assert_equal(0, self.class.hdl.out_length)
    #assert_equal(0, self.class.hdl.err_length)
  end

end
