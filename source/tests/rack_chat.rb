require_relative '../rack/sioux'
require 'rack'

puts "starting rack_chat...."

class RackHandler
    def initialze( app )
        @chat = []
        @app  = app
    end
    
    def call( environment )
        @app.call( environment )
    end
    
    # Bayeux specific interface
    def handshake()
    end
    
    def publish()
    end
    
    # PubSub specific interface
    def validate_node node_name
        node_name == { 'p1' => 'chat' }
    end
    
    def node_init node_name
        @chat
    end
    
    def set_root root
        @root = root
    end
end

app = Rack::Builder.new do
    use RackHandler
end

# Currently under OSX, only the debug version is working
Rack::Handler::Sioux.run( app, 'Environment' => 'debug', 'Host' => 'localhost', 'Port' => 8080 )