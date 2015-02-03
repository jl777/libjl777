require_relative 'exceptions'
require 'open3'
require 'elif'

class App
    def self.start
        btcd_file = Pathname.new(File.join($btcd_binary_folder,'BitcoinDarkd')).expand_path
        raise ApplicationMissingException,"Could not find BitcoinDarkd in path #{btcd_file.to_s}" unless btcd_file.exist?
        puts "Starting BitcoinDarkd"
        cwd = Dir.pwd
        Dir.chdir(btcd_file.dirname)
        job = fork do
            exec "unbuffer #{btcd_file.to_s} > #{$log_file.to_s} &"
        end
        _, status = Process.waitpid2(job)
        Dir.chdir(cwd)
        sleep 5
        if `ps aux | grep -i BitcoinDarkd | grep -v grep` != ""
            # Wait for ready signal in nohup.out file to show 'back from start'
            App.searchLog('back from start', true, 50)
            sleep 5 # Extra time to connect to some peers
        end
        puts "Completely finished starting BitcoinDarkd"
    end

    # Search for the given text in the BitcoinDarkd execution log.
    # The strict search restricts the search to the exact text found
    # somewhere in the log. When it's false a regular experssion is admitted.
    def self.searchLog(text, strict_search = true, timeout = 10)
        old_timeout = timeout
        while timeout > 0
            Elif.open($log_file).each_line do |line|
                if strict_search
                    return line if line.include?(text)
                else
                    return line if line.match(text)
                end
            end
            sleep 1 
            timeout -= 1
        end
        raise LogTimeoutException, "Spent #{old_timeout} seconds waiting for text \"#{text}\" to appear in #{$log_file}"
    end
end
