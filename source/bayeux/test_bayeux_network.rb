# Copyright (c) Torrox GmbH & Co KG. All rights reserved.
# Please note that the content of this file is confidential or protected by law.
# Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

require_relative '../tests/bayeux_reply'
require_relative 'client'
require 'json'
require 'minitest/unit'
require 'thread'


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
 
    def successful message
        message.class == Hash && message.has_key?( 'successful' ) && message[ 'successful' ]
    end
     
    def single_message message
        return nil if message.class != Array || message.length != 1
        message[ 0 ]
    end
    
    def small_client_server_dialog session
        result = single_message( session.send( { 'channel' => '/foo/bar/chu', 'data' => true } ) )
        refute_nil( result, 'response to published data expected' )
        assert( successful( result ) )
    end
    
    def test_simple_subscribe_publish
        Bayeux::Session.start do | session |
            small_client_server_dialog session
        end
    end
 
    def test_mulitple_session_over_multiple_connection
        Array.new( 4 ) do
            Thread.new do
                Bayeux::Session.start { | session | small_client_server_dialog session }
            end 
        end.each { | t | t.join } 
    end

    def test_mulitple_session_over_single_connection
        Bayeux::Connection.connect do | connection |
            Array.new( 4 ) do
                Thread.new do 
                    Bayeux::Session.start( connection ) { | session | small_client_server_dialog session } 
                end
            end.each { | t | t.join } 
        end
    end
    
    def test_one_publish_one_subcribe
        [ Thread.new do
            Bayeux::Session.start do | session |
                result = session.subscribe '/foo/bar'
                
                result = result.concat session.connect until result.lenghth >= 2 
                assert_equal( 2, result.length )
            end
        end,
        Thread.new do
            Bayeux::Session.start do | session |
            end
        end ].each { | t | t.join }
    end
end

