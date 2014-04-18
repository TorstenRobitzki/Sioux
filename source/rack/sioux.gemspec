require 'date'

RACK_GEM_SPEC = Gem::Specification.new do | gem |
    gem.name     = 'sioux'
    gem.version  = '0.6.4'
    
    gem.date     = Date.today.to_s
    
    gem.summary  = 'Comet Rack Server'
    gem.description = 'A Comet Rack Server that allows for publish subscribe on nodes addressed by a key value pair list.'

    gem.homepage = 'https://github.com/TorstenRobitzki/Sioux'
    gem.email    = 'gemmaster@robitzki.de'
    gem.author   = 'Torsten Robitzki'
    
    gem.licenses = ["MIT"]
    gem.files = Dir[ 'lib/**/*' ] + Dir[ 'vendor/**/*' ]
    gem.add_dependency "railties", "~> 3.1"
    puts "files: #{gem.files}"
end