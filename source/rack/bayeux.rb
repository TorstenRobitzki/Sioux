# Copyright (c) Torrox GmbH & Co KG. All rights reserved.
# Please note that the content of this file is confidential or protected by law.
# Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

module Rack
    module Bayeux
        class Handshake
            def initialize app, &handler
                @app     = app
                @handler = handler
            end
            
            def call( env )
                @app.call( env )
            end
        end

        class Publish
            def initialize app, &handler
                @app     = app
                @handler = handler
            end

            def call( env )
                @app.call( env )
            end
        end
    end
end