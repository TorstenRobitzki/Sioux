# Copyright (c) Torrox GmbH & Co KG. All rights reserved.
# Please note that the content of this file is confidential or protected by law.
# Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

require 'minitest/unit'
require_relative 'sioux'

class BayeuxRackTest < MiniTest::Unit::TestCase
    def test_default_parameters_for_all
        Rack::Handler::Sioux.run( [] )
    end
    
    def test_release_environment
        Rack::Handler::Sioux.run( [], 'Environment' => 'release' )
    end
    
    def test_debug_environment
        Rack::Handler::Sioux.run( [], 'Environment' => 'debug' )
    end
    
    def test_unrecognized_parameters_are_detected
        assert_raises( RuntimeError ) { Rack::Handler::Sioux.run( [], 'Foo' => 'Bar' ) }
    end
end