# coding: utf-8
$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))

require 'optparse'
require 'bindata'

require 'pry'

Encoding.default_external = 'utf-8'
Encoding.default_internal = 'utf-8'

# 参考：ms 対応するために無理クリ msオフセット でなんとかできないかな。。。以下の仕様書で。。。
# unsignedで32ビットの場合、最大で0xffffffff(10進数で4294967295)までの数値を扱うことができる。
# これを10ミリ秒単位でカウントアップすると、42949672950ミリ秒、つまり497日2時間27分52秒950ミリ秒までしか数えることができない。
# 従って、この問題のあるシステムでは、497日を超えた連続稼動が出来ない。

class Head < BinData::Record
  endian :little
  uint32 :total_count
  uint32 :start_index
  uint32 :end_index
  uint32 :address
end

class Log < BinData::Record
  endian :little
  uint32 :time
  uint32 :string_address
  uint32 :parameter1
  uint32 :parameter2
end 

def get_binlog(file)
  binlog = Hash.new
  begin
    io = File.open(file)
    binlog[:head] = Head.read(io)
    binlog[:logs] = Array.new
    binlog[:head].total_count.times do
      binlog[:logs] << Log.read(io)
    end
  rescue => e
    raise "get_binlog() error. " + e.message
  end
  return binlog
end

def get_runable_object(file,key)
  runable_object = Hash.new
  begin
    runable_object[:body] = File.binread(file)
    runable_object[:magic_keyword_offset] = runable_object[:body] =~ /#{key}/
    raise "not found magic_keyword '@DOUBLE_MCTWIST@'" if runable_object[:magic_keyword_offset].nil?
  rescue => e
    raise "get_runable_object() error. " + e.message
  end
  return runable_object
end

def get_string(target_address, base_address, runable_object)
  # ex)
  #  target_address  = 1010
  #  base_address    = 1000
  #  runable_address =  100
  runable_address = runable_object[:magic_keyword_offset];
  runable_body = runable_object[:body]
  offset = base_address - runable_address
  pos = target_address - offset
  string = Array.new
  # TODO: 最大256アスキー文字しか考慮しない
  256.times do |index|
    char = runable_body[pos+index]
    break if char == "\x00"
    string << char
  end
  return string.join.force_encoding("cp932"); # TODO: cp932のみ対応
end

def create_log(binlog,runable_object)
  logs = Array.new

  start_index = binlog[:head].start_index
  total_count = (binlog[:head].start_index == 0) ? binlog[:head].end_index : binlog[:head].total_count

  total_count.times do |index|
    blog = binlog[:logs][(start_index + index) % binlog[:head].total_count]
    log = Hash.new
    log[:time] = blog.time
    log[:parameter1] = blog.parameter1
    log[:parameter2] = blog.parameter2
    log[:string] = get_string(blog.string_address, binlog[:head].address, runable_object)
    logs << log
  end
  return logs
end

def output(logs,file)
  out_buffer = Array.new
  logs.each.with_index(0) do |log,index|
    # puts sprintf("%04d:%s:%s:0x%08x(%d),0x%08x(%d)",index, Time.at(log[:time]), log[:string], log[:parameter1],log[:parameter1], log[:parameter2],log[:parameter2])
    out_buffer << sprintf("%s:%s:0x%08x(%d),0x%08x(%d)", Time.at(log[:time]), log[:string], log[:parameter1],log[:parameter1], log[:parameter2],log[:parameter2])
  end
  File.write(file,out_buffer.join("\n"))
end

# フォーマット
# 4byte:件数
# 4byte:開始位置
# 4byte:終了位置(0=ループ発生)
# 4byte:マジックキーワード(@DOUBLE_MCTWIST@)アドレス
# 以後、繰り返しブロック
#  4byte:時間( Time.at(0) = 1970-01-01 09:00:00 +0900 )
#  4byte:文字先頭アドレス
#  4byte:パラメータ１
#  4byte:パラメータ２

# オプション
parameters = Hash.new
parameters[:magic_keyword] = '@DOUBLE_MCTWIST@'
parameters[:endian] = :little
parameters[:encoding] = 'cp932';
opt = OptionParser.new
opt.on('-m VAL', '--magic-keyword VAL') {|val| parameters[:magic_keyword] = val } # マジックキーワード
opt.on('-o VAL', '--outpath VAL') {|val| parameters[:outpath] = val } # 出力先
opt.on('-b', '--big') {parameters[:endian] = :bing } # TODO: エンディアン
opt.on('-e VAL', '--encoding VAL') {|val| parameters[:encoding] = val } # TODO: エンコード


argv = opt.parse(ARGV)
if argv.length != 2 then
  puts "usage: binlog2txt [binlog] [runable object]"
  puts "  options: -m(--magic-keyword): magic keyword. default is '@DOUBLE_MCTWIST@'."
  puts "           -o(--outpath): output file path. default is binlog + '.txt'."
  puts "           -b(--big): endian is big. default is 'little'."
  puts "           -e(--encoding): encoding. default is 'cp932'."
  exit
end

parameters[:binlog_path] = argv[0].encode('utf-8')
parameters[:runable_object_path] = argv[1].encode('utf-8')

# 出力先が未設定ならデフォルト値設定
unless parameters.has_key? :outpath
  parameters[:outpath] = parameters[:binlog_path] + '.txt';
end

begin
  binlog = get_binlog(parameters[:binlog_path])
  runable_object = get_runable_object(parameters[:runable_object_path],parameters[:magic_keyword])
  logs = create_log(binlog,runable_object)
  output(logs,parameters[:outpath])
rescue => e
  puts "ERROR: " + e.message + "\n" + e.backtrace.join("\n")
  exit
end
# File.write(parameters[:outpath],log.join("\n"))

puts "complete."
