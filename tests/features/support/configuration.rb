require 'pathname'
require 'inifile'
require_relative 'exceptions'

class Configuration
    def self.load
        config_file = Pathname.new(File.join(Dir.pwd, 'configuration.ini')).expand_path
        raise ConfigurationFileMissingException unless config_file.exist?
        config = YAML.load_file(config_file)
        raise ConfigurationParameterMissingException unless config.include?('BitcoinDark-binary-folder')
        raise ConfigurationParameterMissingException unless config.include?('BitcoinDark-home-directory')
        raise ConfigurationParameterMissingException unless config.include?('BitcoinDark-configuration-filename')
        # Load compiled application folder
        $btcd_binary_folder = Pathname.new( File.join(Dir.pwd, config['BitcoinDark-binary-folder']) ).expand_path
        # Load username and password from BitcoinDark.conf file
        btcd_config_file = Pathname.new( File.join( config['BitcoinDark-home-directory'], config['BitcoinDark-configuration-filename'] ) ).expand_path
        raise ConfigurationFileMissingException, "I couldn't find BitcoinDarkd configuration file in #{btcd_config_file.to_s} (this path can be configured in #{config_file.to_s})" unless btcd_config_file.exist?
        btcd_config = IniFile.load(btcd_config_file)
        raise ConfigurationParameterMissingException unless btcd_config['global'].include?('rpcuser') && btcd_config['global'].include?('rpcpassword')
        return btcd_config['global']['rpcuser'], btcd_config['global']['rpcpassword']
    end
end

# Initalize configuration
$rpcuser, $rpcpassword = Configuration.load

$log_file = Pathname.new( File.join($btcd_binary_folder, 'btcd.log') ).expand_path