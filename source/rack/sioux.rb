# Copyright (c) Torrox GmbH & Co KG. All rights reserved.
# Please note that the content of this file is confidential or protected by law.
# Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

module Rack
    module Handler
        class Sioux
            POSSIBLE_ENVIRONMENTS = %w{release debug converage}
            DEFAULTS = { 
                'Environment'   => POSSIBLE_ENVIRONMENTS[ 0 ],
                'Host'          => 'localhost',
                'Port'          => 8080
            }
            
            def self.run app, options = {}
                options.each_key do | key |
                    raise "unrecongnized parameters to 'Sioux.run()': #{key} => #{options[ key ]}" unless DEFAULTS.has_key? key
                end
                
                options = DEFAULTS.merge options 
                environment = options.delete 'Environment' 
                raise "unsupported environment #{}" unless POSSIBLE_ENVIRONMENTS.detect environment
                require_relative "../../lib/#{environment}/rack/bayeux"
                
                SiouxRubyImplementation.run app, options
            end

            def self.valid_options
                {
                    "Environment=#{POSSIBLE_ENVIRONMENTS.join('|')}" => "build flavor of the sioux server (default: #{DEFAULTS['Environment']})",
                    "Host=hostname|ip-address" => "address of a single IP endpoint to bind to (default: 'localhost')",
                    "Port=ip-port" => "port of a single IP endpoint to bind to (default: 8080)"
                }
            end
        end
    end
end