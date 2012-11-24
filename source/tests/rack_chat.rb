# Copyright (c) Torrox GmbH & Co KG. All rights reserved.
# Please note that the content of this file is confidential or protected by law.
# Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

$LOAD_PATH << File.expand_path('../../rack/lib', __FILE__)

require 'rack'
require 'rack/handler/sioux'
require 'rack/static'

puts "starting rack_chat...."
puts "connect to server via: \'http://localhost:8080\'"

class Adapter
    @@SAY_CHANNEL  = { 'p1' => 'say' }
    @@CHAT_CHANNEL = { 'p1' => 'chat' } 
    @@MAX_MESSAGES = 20

    def initialize
        @messages = [ 'head' => '?', 'text' => 'Welcome :-)' ]
    end
        
    # this callback is called first. Return true, if the given node names a valid node in your application    
    def validate_node node
        node == @@CHAT_CHANNEL 
    end

    # this callback is called after the node was validated. Return true, if the subscriber is allowed to subscribe to
    # the node    
    def authorize subscriber, node
        true
    end
     
    # This callback is called after validation and authorization. return the initial data wrapped in an hash with 
    # 'data' field. An additional 'id' field will be transported to the clients too
    def node_init node
        { 'data' => @messages }
    end
    
    # This callback will be invoked, when a client invokes cometd.publish()
    def publish node, data, root 
        return [ false, 'invalid channel' ] unless node == @@SAY_CHANNEL
        
        @messages.shift if @messages.size == @@MAX_MESSAGES 
        @messages << { 'head' => '?', 'text' => data }

        root[ @@CHAT_CHANNEL ] = { 'data' => @messages }

        [ true, '' ]
    end
end

ROOT = File.expand_path(File.dirname(__FILE__))

app = Rack::Builder.new do
    use Rack::Static, :urls => [ '/jquery' ], :root => ROOT
    use Rack::Static, :urls => [ '/' ], :index => '/index.html', :root => File.join( ROOT, 'chat' )
    run lambda { | env | [ 404, { 'Content-Type' => 'text/html' }, [] ] }
end

Rack::Handler::Sioux.run( app, 'Host' => 'localhost', 'Port' => 8080, 'Adapter' => Adapter.new )

