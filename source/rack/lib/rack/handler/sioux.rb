# Copyright (c) Torrox GmbH & Co KG. All rights reserved.
# Please note that the content of this file is confidential or protected by law.
# Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

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
                'Host'          => 'localhost',
                'Port'          => 8080,
                'Adapter'       => nil
            }
            
            def self.run app, options = {}
# TODO:             
                options.each_key do | key |
#                    raise "unrecongnized parameters to 'Sioux.run()': #{key} => #{options[ key ]}" unless DEFAULTS.has_key? key
                end
                
                options = DEFAULTS.merge options 
                require 'bayeux_sioux'

                server = Rack::Sioux::SiouxRubyImplementation.new
                server.run ApplicationWrapper.new( app, options ), options 
            end

            def self.valid_options
                {
                    "Host=hostname|ip-address" => "address of a single IP endpoint to bind to (default: #{DEFAULTS['localhost']})",
                    "Port=ip-port" => "port of a single IP endpoint to bind to (default: #{DEFAULTS['Port']})",
                    'Adapter=object' => 'object validate and authorize reading access to the root-data object. (default: nil)' 
                }
            end
        end
    end
end