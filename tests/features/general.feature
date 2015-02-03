Feature: General requests and answers from superNET API
    Get peers
    Ping/Pong
    SendMessage/ReceiveMessage

  @general @peers @general-001
  Scenario: As a superNET user I want to see peers on my network
    Given superNET is running on the computer
    Then I receive a list of peers with at least two peers

  @general @ping @general-002
  Scenario: As a superNET user I want to make a ping and receive an answer from a random node
    Given superNET is running on the computer with some peers
    When I send a ping request to one random peer
    Then I receive a pong answer

  @general @ping @general-003
  Scenario: As a superNET user I want to make a ping and receive an answer from a known host
    Given superNET is running on the computer with some peers
    When I send a ping request to "209.126.70.170" peer
    Then I receive a pong answer

  @general @send_message @general-004
  Scenario: As a superNET user I want to send a message to myself and receive it
    Given superNET is running on the computer with some peers
    When I send a message to myself with content "Cucumber SuperNET test"
    Then I receive a message to myself with content "Cucumber SuperNET test" in less than 50 seconds