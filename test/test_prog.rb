#
#  test_prog.rb
#
#  Copyright (c) 2021 by Daniel Kelley
#
# frozen_string_literal: true

require 'test/unit'

require_relative 'cgr101'
require_relative 'cgr101_core'
require_relative 'cgr101_meas'
require_relative 'cgr101_scope'
require_relative 'cgr101_wave'

#
# CGR101 test cases using stdio command interface
#
class TestProg < Test::Unit::TestCase

  include CGR101Core
  include CGR101Meas
  include CGR101Scope
  include CGR101Wave

  # Make sure each test starts in a known state
  def setup
    self.class.hdl.send("*RST")
  end

  # Make sure there is no error at the conclusion of each test
  def cleanup
    # no error reports
    self.class.hdl.send("SYSTem:ERRor:COUNt?")
    out = self.class.hdl.recv
    assert_equal("0", out)
    assert_equal(0, self.class.hdl.out_length)
    assert_equal(0, self.class.hdl.err_length)
  end

  class << self
    attr_reader :hdl

    def startup
      @hdl = CGR101.new
      # @hdl.send("SYSTem:INTernal:CALibrate")
      # @hdl.send("SYSTem:INTernal:CONFigure")
    end

    def shutdown
      @hdl.send("SYSTem:INTernal:quit")
      @hdl.close
    end

  end
end
