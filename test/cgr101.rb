#
#  cgr101.rb
#
#  Copyright (c) 2021 by Daniel Kelley
#

require 'open3'
require 'timeout'

#require 'thread'

#
# CGR101 SCPI application wrapper
#
class CGR101

  attr_reader :wthr
  PROG = ENV['CGR101_PROG'] || 'src/cgr101-scpi'
  SWITCHES = ENV['CGR101_TEST'] || '-DE' # Default: Emulation
  RECV_TIMEOUT = 5
  CMDLOG = ENV['CGR101_CMDLOG']

  def initialize(arg=nil)
    cmd = "#{PROG} #{SWITCHES}"
    if !arg.nil?
      cmd += " $arg"
    end
    #puts cmd
    @stdin, @stdout, @stderr, @wthr = Open3.popen3(cmd)
    @outq = Queue.new
    @errq = Queue.new
    @outr = true
    @errr = true
    @outt = Thread.new { out_reader }
    @errt = Thread.new { err_reader }
    @cmdlog = CMDLOG.nil? ? nil : File.open(CMDLOG, "w")
  end

  def close
    @outr = false
    @errr = false
    @outt.exit
    @errt.exit
    Process.kill("HUP", @wthr.pid)
    @stdin.close
    @stdout.close
    @stderr.close
    @wthr.exit
    return @wthr.value
  end

  def cmdlog(str)
    if !@cmdlog.nil?
      @cmdlog.puts str
    end
  end

  def send(str)
    cmdlog(str)
    @stdin.puts str
  end

  def recv
    Timeout::timeout(RECV_TIMEOUT) do
      return @outq.pop
    end
    nil
  end

  def recv_err
    Timeout::timeout(RECV_TIMEOUT) do
      return @errq.pop
    end
    nil
  end

  def out_length
    @outq.length
  end

  def err_length
    @errq.length
  end

  def get_out
    get_q(@outq)
  end

  def get_err
    get_q(@errq)
  end

  private

  def out_reader
    while @outr do
      @outq << @stdout.gets.chomp
    end
  end

  def err_reader
    while @errr do
      @errq << @stderr.gets.chomp
    end
  end

  def get_q(q)
    a = []
    q.length.times { a << q.pop }
    a
  end

end
