$LOAD_PATH << File.expand_path('../../rack/lib', __FILE__)

require 'rack'
require 'rack/handler/sioux'
require 'rack/static'
require 'pathname'

puts "starting rack_chat...."
puts "connect to server via: \'http://localhost:8080\'"

class Adapter
    @@CHAT_CHANNEL = { 'channel' => 'chat' } 
    @@MAX_MESSAGES = 20

    def initialize
        @messages = [ 'head' => '?', 'text' => 'Welcome :-)' ]
    end
        
    # this callback is called first. Return true, if the given node names a valid node in your application    
    def validate_node node
        node == @@CHAT_CHANNEL 
    end

    # this callback is called after the node was validated. Return true, if the subscriber is allowed to subscribe to
    # the node. If authorization is disabled by configuration, this function is not required.
    def authorize subscriber, node
        true
    end
     
    # This callback is called after validation and authorization. return the initial data.
    def node_init node
        @messages
    end
    
    # This callback will be invoked, when a client invokes cometd.publish()
    def publish message, root
        puts "message: #{message.inspect}"
        puts "text: #{message['text']}"
        @messages.shift if @messages.size == @@MAX_MESSAGES 
        @messages << { 'head' => '?', 'text' => message[ 'text' ] }

        root[ @@CHAT_CHANNEL ] = @messages

        [ true, 200 ]
    end
end

ROOT = File.expand_path(File.dirname(__FILE__))

app = Rack::Builder.new do
    use Rack::Static, :urls => [ '/jquery' ], :root => ROOT
    use Rack::Static, :urls => [ '/pubsub_http' ], :root => Pathname( ROOT ).parent
    use Rack::Static, :urls => [ '/' ], :index => '/index.html', :root => File.join( ROOT, 'chat' )
    run lambda { | env | [ 404, { 'Content-Type' => 'text/html' }, [] ] }
end

Rack::Handler::Sioux.run( app, 'Host' => 'localhost', 'Port' => 8080, 'Adapter' => Adapter.new )

