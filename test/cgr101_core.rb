#
#  cgr101_core.rb
#
#  Copyright (c) 2022 by Daniel Kelley
#

require 'pp'

#
# Command test cases
#
module CGR101Core

  #
  # Interface handle check
  #
  def test_core_001
    hdl = self.class.hdl
    assert_not_nil hdl
    assert hdl.wthr.alive?
  end

  #
  # Device Setup completed correctly
  #
  def test_core_002
    self.class.hdl.send("SYSTem:INTernal:SETUP?")
    out = self.class.hdl.recv
    assert_equal("0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard SYST:VERS?
  #
  def test_core_003
    self.class.hdl.send("SYST:VERS?")
    out = self.class.hdl.recv
    assert_equal("1999.0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard SYSTem:ERRor:COUNt?
  #
  def test_core_004
    self.class.hdl.send("SYSTem:ERRor:COUNt?")
    out = self.class.hdl.recv
    assert_equal("0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard SYSTem:ERRor?
  #
  def test_core_005
    self.class.hdl.send("SYSTem:ERRor?")
    out = self.class.hdl.recv
    assert_equal("0,\"No error\"", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # "No error" command behavior
  #
  def test_core_006
    self.class.hdl.send("SYSTem:ERRor:NEXT?")
    out = self.class.hdl.recv
    assert_equal("0,\"No error\"", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # "Has error" command behavior
  #
  def test_core_007
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
  def test_core_008
    self.class.hdl.send("SYSTem:CAPability?")
    out = self.class.hdl.recv
    assert_equal("\"DIGITIZER\"", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard *CLS
  #
  def test_core_009
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
  def test_core_010
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
  def test_core_011
    self.class.hdl.send("*ESR?")
    out = self.class.hdl.recv
    assert_not_nil(out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard *IDN?
  #
  def test_core_012
    self.class.hdl.send("*idn?")
    out = self.class.hdl.recv
    # Don't match against specific firmware revision
    assert_match(/^GMP,CGR101-SCPI,1.0,.*CircuitGear/, out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard *OPC, *OPC?, *ESR clearing
  #
  def test_core_013
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
  def test_core_014
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
  def test_core_015_001
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
  def test_core_016_001
    self.class.hdl.send("*stb?")
    out = self.class.hdl.recv
    assert_equal("0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard *TST?
  #
  def test_core_016_002
    self.class.hdl.send("*tst?")
    out = self.class.hdl.recv
    assert_equal("0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard *WAI
  #
  def test_core_016_003
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
  def test_core_016_004
    self.class.hdl.send("STATus:OPERation?")
    out = self.class.hdl.recv
    assert_equal("0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard STATus:OPERation:EVENt?
  #
  def test_core_016_005
    self.class.hdl.send("STATus:OPERation:EVENt?")
    out = self.class.hdl.recv
    assert_equal("0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard STATus:OPERation:CONDition?
  #
  def test_core_017
    self.class.hdl.send("STATus:OPERation:CONDition?")
    out = self.class.hdl.recv
    assert_equal("0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard STAT:OPER:ENAB?, STAT:OPER:ENAB
  #
  def test_core_018
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
  def test_core_019
    self.class.hdl.send("STATus:QUEStionable?")
    out = self.class.hdl.recv
    assert_equal("0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # SCPI standard STAT:QUES:ENAB?, STAT:QUES:ENAB
  #
  def test_core_020
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
  def test_core_021
    self.class.hdl.send("STATus:PRESet")
    self.class.hdl.send("*opc?")
    out = self.class.hdl.recv
    assert_equal("1", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  #
  # Command prefix
  #
  def test_core_022
    self.class.hdl.send("SYST:ERR:COUN?;COUN?;COUN?")
    out = self.class.hdl.recv
    assert_equal("0;0;0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  def test_core_023
    self.class.hdl.send(":SYST:ERR?")
    out = self.class.hdl.recv
    assert_equal("0,\"No error\"", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  def test_core_024
    self.class.hdl.send("SYST:ERR:COUN?;:SYST:ERR:COUN?;COUN?")
    out = self.class.hdl.recv
    assert_equal("0;0;0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  def no_test_core_outline
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
