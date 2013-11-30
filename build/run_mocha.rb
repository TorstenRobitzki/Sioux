
def mocha_test_task test_name, *options 
    params = { :sources => [] }
    options.each { | p | params.merge! p }
    sources    = params.delete( :sources ).to_s

    task test_name.to_sym do
        cmd = "mocha --compilers coffee:coffee-script #{sources}"
        result = system cmd
        
        raise "#{test_name} failed!" unless result
    end
end