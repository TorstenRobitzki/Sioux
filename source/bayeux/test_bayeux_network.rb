# Copyright (c) Torrox GmbH & Co KG. All rights reserved.
# Please note that the content of this file is confidential or protected by law.
# Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

require_relative '../tests/bayeux_reply'
require_relative 'network_test_cases'
require 'minitest/unit'

class BayeuxProtocolTest < MiniTest::Unit::TestCase
    def setup 
        @server = BayeuxServerProxy.new
    end

    def teardown
        begin
            @server.stop
        rescue
        end
    end
 
    include BayeuxNetworkTestcases    
end

