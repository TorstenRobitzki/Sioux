require 'rack'
require 'rack/lint'
require 'net/http'


## 
# This class serves as a rack application for testing.
class RackTestApplication
    attr_reader :environment
    
    def call environment
        raise StopIteration if environment[ 'PATH_INFO' ] == '/stop'
        raise RuntimeError, "raised for testing"  if environment[ 'PATH_INFO' ] == '/throw'
        
        if environment[ 'PATH_INFO' ] == '/errors'
            log = environment[ 'rack.errors' ]
            log.puts 4
            log.write "=2+2"
            log.flush
        end        
        
        @environment = environment
        [ 200, {"Content-Type" => "text/html"}, [ "Hello Rack!" ] ]
    end
end

##
# Module that provides a setup and teardown function to start and stop a sioux rack server with a very simple application
module SetupRackserver
    @@HOST      = '127.0.0.1'
    @@PORT      = 8080
    @@URI       = URI( "http://#{@@HOST}:#{@@PORT}" )
    
    def wait_until_started to
        now  = Time.now
        stop = false
        
        while Time.now < now + to and !stop do
            begin
                s = TCPSocket.new @@HOST, @@PORT
                s.close
                stop = true
            rescue
            end
        end
        
        raise "Timeout waiting for sever!" unless stop
    end

    def setup adapter = nil, handler = nil, o = { 'Protocol' => 'bayeux' }
        options = { 
            'Environment'   => 'debug', 
            'Host'          => @@HOST, 
            'Port'          => @@PORT, 
            'Adapter'       => adapter }.merge o
        
        @app = handler || RackTestApplication.new
        @server_loop = Thread.new do
            begin
                Rack::Handler::Sioux.run( Rack::Lint.new( @app ), options )
            rescue => error
                puts "Error Rack::Handler::Sioux.run() #{error.message}"
                puts error.backtrace.join("\n")        
            end
        end

        wait_until_started 2
    end

    def ignore_errors
        begin
            yield
        rescue
        end
    end

    def stop_server
        ignore_errors { Net::HTTP.get( @@URI + '/stop' ) }
    end
    
    def join_server
        @server_loop.join
    end
    
    def teardown
        stop_server     
        join_server   
    end
end
