# Copyright (c) Torrox GmbH & Co KG. All rights reserved.
# Please note that the content of this file is confidential or protected by law.
# Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

require_relative '../rack/sioux'
require_relative '../rack/bayeux'
require 'rack'
require 'rack/debug'
require 'rack/static'

puts "starting rack_chat...."
messages = []

ROOT = File.expand_path(File.dirname(__FILE__))

app = Rack::Builder.new do
    use Rack::Lint

    use Rack::Bayeux::Handshake do | env, ext, session |
    end
    
    use Rack::Bayeux::Publish do | env, channel, data, whole_message, session_data, root |
        return [ false, 'invalid channel' ] unless channel == '/chat'
        root[ { 'P1' => 'chat' } ] = messages
        [ true, '' ]
    end
    
    use Rack::Sioux::Validate do | env, node |
        node == { 'P1' => 'chat' } 
    end
    
    use Rack::Sioux::Authorize do | env, node |
        true
    end
     
    use Rack::Sioux::Initialize do | env, node |
        messages
    end

    use Rack::Lint
    use Rack::Static, :urls => [ '/jquery' ], :root => ROOT
    use Rack::Static, :urls => [ '/' ], :index => '/index.html', :root => File.join( ROOT, 'chat' )
    run lambda { | env | [ 404, { 'Content-Type' => 'text/html' }, [] ] }
end

Rack::Handler::Sioux.run( app, 'Environment' => 'debug', 'Host' => 'localhost', 'Port' => 8080 )

