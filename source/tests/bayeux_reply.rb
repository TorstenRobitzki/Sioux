# Copyright (c) Torrox GmbH & Co KG. All rights reserved.
# Please note that the content of this file is confidential or protected by law.
# Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

require 'thread'
require 'net/http'

class BayeuxServerProxy
    def initialize

        @server_thread = Thread.new do 
            if block_given?
                yield
            else
                Rake::Task['bayeux_reply'].execute()
            end    
        end

                
        while !server_is_running() do
            puts "waiting for reply server...." 
        end
    end

    def stop
        begin
            Net::HTTP.get( URI( 'http://localhost:8080/stop' ) )
        rescue EOFError
        end
        
        @server_thread.join        
    end
    
private
    def server_is_running
        sleep 0.5
        result = nil
        begin
            result = Net::HTTP.get( URI( 'http://localhost:8080/ping' ) )
            puts result
        rescue EOFError
            result = "running"
        rescue
            puts "connecting reply server: #{ $! }"
        end
        
        result
    end
    
end
