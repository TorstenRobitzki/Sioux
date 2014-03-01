require_relative 'lib/rack/handler/sioux'
require_relative './setup_rack_server.rb'

class Adapter 
end

class Server 
    include SetupRackserver
end

s = Server.new

puts ARGV.inspect
if ARGV[ 0 ] == 'start'
    s.setup Adapter.new, nil, 'Protocol' => 'pubsub'
   
    s.join_server    
else
    s.stop_server
end        