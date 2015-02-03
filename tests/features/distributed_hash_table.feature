Feature: Distributed Hash Table (Kademlia DHT) requests and answers from superNET API
    Find Nodes

  @dht @find_nodes @dht-001
  Scenario: As a superNET user I want to request information to find more nodes on the network
    Given superNET is running on the computer
    And I receive a list of peers with at least two peers
    When I request one of my peers for information to find more nodes
    Then I see a request for information to find more nodes in the log