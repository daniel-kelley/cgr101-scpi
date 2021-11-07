#
#  test_prog.rb
#
#  Copyright (c) 2021 by Daniel Kelley
#
# frozen_string_literal: true

require 'test/unit'

require_relative 'cgr101'
require_relative 'cgr101_testcase'

#
# CGR101 test cases using stdio command interface
#
class TestProg < Test::Unit::TestCase

  include CGR101Testcase

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
