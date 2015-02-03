Before do |scenario|
  # The +scenario+ argument is optional, but if you use it, you can get the title,
  # description, or name (title + description) of the scenario that is about to be
  # executed.
  File.open($log_file, 'w') {|file| file.truncate(0) } if $log_file.exist?
end