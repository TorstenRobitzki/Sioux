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
        if message.class == Array then
            message.detect{ | ele | !successful ele }.nil?
        else 
            message.class == Hash && message.has_key?( 'successful' ) && message[ 'successful' ]
        end
    end
    
    def assert_for_update response, subject, expected_result
        result = response.detect do | message|        
            message.has_key?( 'channel' ) && message[ 'channel' ] == subject &&
                message.has_key?( 'data' ) && message[ 'data' ] == expected_result
        end

        assert( result, "expected update to subject #{subject} with a value of #{expected_result}\ngot: #{response}\n" )   
        
        result      
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
        signal = Queue.new
        
        [ Thread.new do
            Bayeux::Session.start do | session |
                result = session.subscribe '/foo/bar'
                
                result = result.concat session.connect until result.length >= 1 
                assert_equal( 1, result.length )
                assert( successful result )
                
                signal << 42

                result = session.connect
                assert_equal( 1, result.length )
                assert_for_update( result, '/foo/bar', 'new_value')
            end
        end,
        Thread.new do
            Bayeux::Session.start do | session |
                signal.pop
                
                result = session.publish '/foo/bar', 'new_value'
                assert_equal( 1, result.length )
                assert( successful result )
            end
        end ].each { | t | t.join }
    end

    def test_one_publish_multiple_subcribe
        signal = Queue.new
        subscriber_count = 100
        
        [ (1..subscriber_count).collect do | index | 
            Thread.new( index ) do | i |
                Bayeux::Session.start do | session |
                    result = session.subscribe '/foo/bar'

                    while result.empty? do
                        result = result.concat session.connect 
                    end
                    
                    assert_equal( 1, result.length )
                    assert( successful result )
                    
                    signal << 42
    
                    result = session.connect
                    assert_equal( 1, result.length )
                    assert_for_update( result, '/foo/bar', 'new_value')
                end
            end
        end,
        Thread.new do
            subscriber_count.times { signal.pop }

            Bayeux::Session.start do | session |
                result = session.publish '/foo/bar', 'new_value'
                assert_equal( 1, result.length )
                assert( successful result )
            end
        end ].flatten.each { | t | t.join }
    end
end

