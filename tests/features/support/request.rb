#!/usr/bin/env ruby
require 'json'
require 'net/http'
require 'pathname'
require_relative 'exceptions'
require_relative 'configuration'

puts "Using Ruby version #{RUBY_VERSION}"

class SuperNETTest
    def self.requestSuperNET(user, password, request, url = '127.0.0.1', port = '14632')
        uri = URI.parse("http://#{url}:#{port}")
        http = Net::HTTP.new(uri.host, uri.port)
        req = Net::HTTP::Post.new(uri.request_uri)
        req.body = {
            :jsonrpc => "1.0",
            :id => "rubytest",
            :method => "SuperNET",
            :params => [ request ]
        }.to_json
        req.content_type = 'text/plain'
        req.basic_auth(user, password)
        res = http.request(req)
        raise EmptyResponseException if res.body.nil?
        raise EmptyResponseResultException unless JSON.parse(res.body).include?('result')
        raise EmptyResponseResultException, "Response received: #{res.body}" if JSON.parse(res.body)['result'].nil?
        # Store the last response in case it is needed in following steps
        $last_response = JSON.parse(res.body)
        return JSON.parse(JSON.parse(res.body)['result'])
    end
end