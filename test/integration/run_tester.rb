#!/usr/bin/env ruby

puts "Peran Integration Tester"
puts "\e[32m[==========]\e[0m Running testcases under test/integration/cases/"


total = 0
Dir.glob('./cases/*') do |fileName|
  succeeded = true
  basename = File.basename(fileName, '.pr')
  puts "\e[32m[----------]\e[0m"
  puts "\e[32m[ RUN      ]\e[0m #{basename}"
  pr = "#{fileName}"
  native = "./compiled/#{basename}"
  actual = "./actual/#{basename}.txt"
  expected = "./expected/#{basename}.txt"

  commands = ["PERYAN_RUNTIME_PATH=../../runtime ../../src/peryan #{pr} #{native}",
              "#{native} > #{actual}"]
  commands.each do |command|
    if not system(command)
      puts "\e[31m[          ]\e[0m Error while executing: #{command}"
      succeeded = false
      break
    end
  end
  
  if succeeded
    actual_text = File.open(actual).read
    expected_text = File.open(expected).read

    if actual_text != expected_text
      puts "Error: assertion failed:"
      puts "Actual: #{actual_text}"
      puts "Expected: #{expected_text}"
      succeeded = false
    end
  end

  if succeeded
    puts "\e[32m[       OK ]\e[0m #{File.basename(fileName, '.pr')}"
    puts "\e[32m[----------]\e[0m"
  else
    puts "\e[31m[   FAILED ]\e[0m #{File.basename(fileName, '.pr')}"
    puts "\e[31m[----------]\e[0m"
    total += 1
  end
end

if total == 0 then
  puts "\e[32m[==========]\e[0m All integration tests succeeded."
else
  puts "\e[31m[==========]\e[0m #{total} integration test(s) failed"
end
