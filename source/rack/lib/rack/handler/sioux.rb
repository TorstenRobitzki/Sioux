require 'stringio'
require 'json'
require 'rack'

module Rack
    module Handler
        class ApplicationWrapper
            def initialize app, options
                raise ArgumentError if app.nil?
                @app     = app
                @options = options
            end
             
            def call environment
                error_log = StringIO.new '', 'w'
                
                # patch the environment
                environment[ 'SERVER_PORT' ] = environment[ 'SERVER_PORT' ].to_s 
                environment[ 'rack.input' ]  = StringIO.new environment[ 'rack.input' ], 'r'
                environment[ 'rack.errors' ] = error_log 
                environment[ 'rack.version' ] = Rack::VERSION
                                
                begin
                    status, headers, body = @app.call environment
    
                    # patch the result to be more convenience for the c++ part
                    body_text = ""
                    body.each { | line | body_text = body_text + line }
                    body.close if body.respond_to? :close                        
                                        
                    if !body_text.empty? && !headers.include?( 'Content-Length' )
                        headers[ 'Content-Length' ] = body_text.length 
                    end
                    
                    header_text = headers.inject( "" ) do | sum, ( header, value ) | 
                        sum + "#{header}: #{value}\r\n"
                    end + "\r\n"
                    
                    [ status.to_i, header_text, body_text, error_log.string ]
                rescue StopIteration
                    []
                end
            end
        end
        
        class Sioux
            POSSIBLE_ENVIRONMENTS = %w{release debug converage}
            DEFAULTS = { 
                'Host'                          => 'localhost',
                'Port'                          => 8080,
                'Adapter'                       => nil,
                'Pubsub.max_update_size'        => 0,
                'Pubsub.authorization_required' => true,
                'Bayeux.max_messages_size_per_client' => 10 * 1024
            }
            
            def self.to_bool s
                s == true || s == 'yes'
            end
            
            def self.run app, options = {}
                options = Hash[ options.collect{ |k,v| [k.to_s, v ] } ]                
                options = DEFAULTS.merge options 

                # boolean options 
                [ 'Pubsub.authorization_required' ].each do | bool_option |
                    options[ bool_option ] = to_bool options[ bool_option ]
                end

                require 'bayeux_sioux'

                server = Rack::Sioux::SiouxRubyImplementation.new
                server.run ApplicationWrapper.new( app, options ), options 
            end

            def self.valid_options
                {
                    "Host=hostname|ip-address" => "address of a single IP endpoint to bind to (default: #{DEFAULTS['localhost']})",
                    "Port=ip-port" => "port of a single IP endpoint to bind to (default: #{DEFAULTS['Port']})",
                    'Adapter=object' => 'object validate and authorize reading access to the root-data object. (default: nil)',
                    'Pubsub.max_update_size=SIZE' => 'the ratio of update costs to full nodes data size in %. (default: 0)',
                    'Pubsub.authorization_required=yes|no' => 'describes whether or not a reading node access must be authorized. (default: yes)',
                    'Bayeux.max_messages_size_per_client=SIZE' => 'maximum size of messages, that will be buffered for a client before messages will be discard. (default: 10240)'
                }
            end
        end
    end
end