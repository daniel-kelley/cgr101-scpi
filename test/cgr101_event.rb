#
#  cgr101_event.rb
#
#  Copyright (c) 2022 by Daniel Kelley
#

#
# Event Interfaces
#
# | CONFigure:DIGital:EVENt |
# | FETCh:DIGital:EVENt?    |
# | MEASure:DIGital:EVENt?  |
# | READ:DIGital:EVENt?     |

module CGR101Event

  #
  # Low Level Smoke Test
  #

  def test_event_001
    omit_if(!test_jig?)
    self.class.hdl.send("ABOR")
    self.class.hdl.send("CONF:DIG:EVEN POS 4 (@4,6)")
    self.class.hdl.send("INIT")
    self.class.hdl.send("STAT:OPER:COND?")
    out = self.class.hdl.recv
    v0 = out.split(',').map { |s| Integer(s) }
    assert_equal(1, v0.length)
    assert_equal(256, v0[0])

    100.times do |n|
      self.class.hdl.send("SOUR:DIG:DATA 255")
      # sleep 0.1
      #self.class.hdl.send("SYSTem:ERRor?")
      #out = self.class.hdl.recv
      #assert_equal("0,\"No error\"", out, n)
      self.class.hdl.send("STAT:OPER:COND?")
      out = self.class.hdl.recv
      v0 = out.split(',').map { |s| Integer(s) }
      assert_equal(1, v0.length)
      break if v0[0] == 0


      self.class.hdl.send("SOUR:DIG:DATA 0")
      # sleep 0.1
      #self.class.hdl.send("SYSTem:ERRor?")
      #out = self.class.hdl.recv
      #assert_equal("0,\"No error\"", out, n)
      self.class.hdl.send("STAT:OPER:COND?")
      out = self.class.hdl.recv
      v0 = out.split(',').map { |s| Integer(s) }
      assert_equal(1, v0.length)

      break if v0[0] == 0
    end

    assert_equal(0, v0[0])
    self.class.hdl.send("FETC:DIG:EVEN?")
    out = self.class.hdl.recv
    v1 = out.split(',')
    assert_equal(8, v1.length)
    self.class.hdl.send("SYSTem:ERRor?")
    out = self.class.hdl.recv
    assert_equal("0,\"No error\"", out)

  end

end
