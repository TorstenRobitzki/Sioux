require_relative 'lib/rack/handler/sioux'
require_relative './setup_rack_server.rb'

class Adapter 
    def validate_node node
        true
    end
    
    def authorize user, node
        true
    end
    
    def node_init node
        "Test-Data"
    end
    
    def publish message, root
        if message.key? 'update'
            root[ message[ 'update' ] ] = message[ 'value' ]
            [ 'ok', 200 ]
        elsif message.key? 'echo'
            [ message[ 'echo' ], 200 ]
        else
            [ 'error', 500 ]            
        end            
    end
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