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

#
# CGR101 test cases using stdio command interface
#
class TestProg < Test::Unit::TestCase

  include CGR101Core
  include CGR101Meas
  include CGR101Scope

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
