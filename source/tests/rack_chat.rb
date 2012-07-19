# Copyright (c) Torrox GmbH & Co KG. All rights reserved.
# Please note that the content of this file is confidential or protected by law.
# Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

require_relative '../rack/sioux'
require 'rack'
require 'rack/static'

puts "starting rack_chat...."

class Adapter
    @@SAY_CHANNEL  = { 'p1' => 'say' }
    @@CHAT_CHANNEL = { 'p1' => 'chat' } 
    @@MAX_MESSAGES = 20

    def initialize
        @messages = []
    end
        
    def validate_node node
        node == @@CHAT_CHANNEL 
    end
    
    def authorize subscriber, node
        true
    end
     
    def node_init node
        @messages
    end
    
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

Rack::Handler::Sioux.run( app, 'Environment' => 'debug', 'Host' => 'localhost', 'Port' => 8080, 'Adapter' => Adapter.new )

