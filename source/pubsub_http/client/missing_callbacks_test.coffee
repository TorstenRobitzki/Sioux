assert = require( 'chai' ).assert

require './missing_callbacks.coffee'
require './test_tools.coffee'

update_at       = 1
delete_at       = 2
insert_at       = 3
delete_range    = 4
update_range    = 5
edit_at         = 6

describe "pubsub.http interface", ->

    describe "given a callback set that is missing the update_range callback", ->

        update = ( path, num_elements, update_array )->
            log =[]
            path_copy = path.slice()
            PubSub.add_missing_callbacks( update_logger( log, [ 'update_range' ] ) ).update_range( path, num_elements, update_array )

            assert.deepEqual path, path_copy
            log

        describe "and the new range is empty", ->

            it 'yields a range_delete', ->
                assert.deepEqual(
                    update( [ 1, 2 ], 2, [] ),
                    [ [ 'delete_range', [ 1, 2 ], 2 ] ] )

        describe "and the new range is shorter than the original one", ->

            it 'replaces some by updates and removes the rest with range_delete', ->
                assert.deepEqual(
                    update( [ 1, 2 ], 2, [ 'A' ] ),
                    [ [ 'delete_range', [ 1, 3 ], 1 ], [ 'update', [ 1, 2 ], 'A' ] ] )

        describe "and the new range is larger than the original one", ->

            it 'replaces some by updates and adds the rest with inserts', ->
                assert.deepEqual(
                    update( [ 0 ], 2, [ 'A', 'B', 'C', 'D' ] ),
                    [ [ 'update', [ 0 ], 'A' ], [ 'update', [ 1 ], 'B' ], [ 'insert', [ 2 ], 'C' ], [ 'insert', [ 3 ], 'D' ] ] )

        describe "and the replace range is empty", ->

            it 'adds the content with inserts', ->
                assert.deepEqual(
                    update( [ 7 ], 0, [ 'A', 'B' ] ),
                    [ [ 'insert', [ 7 ], 'A' ], [ 'insert', [ 8 ], 'B' ] ] )

        describe "and the new range is as long as the original one", ->

            it 'yields one update for every element', ->
                assert.deepEqual(
                    update( [ 1 ], 3, [ 'A', 'B', 'C' ] ),
                    [ [ 'update', [ 1 ], 'A' ], [ 'update', [ 2 ], 'B' ], [ 'update', [ 3 ], 'C' ] ] )


    describe "given a callback set that is missing the delete_range callback", ->

        update = ( path, num_elements, update_array )->
            log =[]
            path_copy = path.slice()
            PubSub.add_missing_callbacks( update_logger( log, [ 'delete_range' ] ) ).delete_range( path, num_elements )

            assert.deepEqual path, path_copy
            log

        it "substitutes the call by multiple calls to delete", ->
            assert.deepEqual(
                update( [ 1, 3 ], 2 ),
                [ [ 'delete', [ 1, 3 ] ], [ 'delete', [ 1, 3 ] ] ] )

    describe "given a callback set that is missing the update_range and delete_range callback", ->

        update = ( path, num_elements, update_array )->
            log =[]
            path_copy = path.slice()
            PubSub.add_missing_callbacks( update_logger( log, [ 'update_range', 'delete_range' ] ) ).update_range( path, num_elements, update_array )

            assert.deepEqual path, path_copy
            log

        describe "and the new range is empty", ->

            it 'yields a range_delete', ->
                assert.deepEqual(
                    update( [ 1, 2 ], 2, [] ),
                    [ [ 'delete', [ 1, 2 ] ], [ 'delete', [ 1, 2 ] ] ] )

        describe "and the new range is shorter than the original one", ->

            it 'replaces some by updates and removes the rest with range_delete', ->
                assert.deepEqual(
                    update( [ 1, 2 ], 2, [ 'A' ] ),
                    [ [ 'delete', [ 1, 3 ] ], [ 'update', [ 1, 2 ], 'A' ] ] )

    describe "given a callback set that is missing the update callback", ->

        update = ( path, element )->
            log =[]
            path_copy = path.slice()
            PubSub.add_missing_callbacks( update_logger( log, [ 'update' ] ) ).update( path, element )

            assert.deepEqual path, path_copy
            log

        it 'substitutes an update with a delete and an insert', ->
            assert.deepEqual(
                update( [ 1, 2 ], 'F' ),
                [ [ 'delete', [ 1, 2 ] ], [ 'insert', [ 1, 2 ], 'F' ] ] )
