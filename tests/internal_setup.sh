#!/bin/sh
rvm requirements
rvm install ruby

echo "Installing Cucumber (behavioural testing framework) and other libraries required for testing"
gem update
# to automatically install missing gems
bundle install
