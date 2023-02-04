# CN-Final-CA
![Screenshot (330)](https://user-images.githubusercontent.com/80702342/216783081-d9ef9d74-8096-48cf-a00a-a1dc9f818015.png)
![Screenshot (331)](https://user-images.githubusercontent.com/80702342/216783088-ffd46fb8-5a26-4784-9b35-a699aa341e36.png)
![Screenshot (332)](https://user-images.githubusercontent.com/80702342/216783626-cf40e98c-7d9a-4c06-be2e-112580061f88.png)
![Screenshot (333)](https://user-images.githubusercontent.com/80702342/216783633-c21888f9-9e47-4289-b13c-518c33c80839.png)

Distance Vector Routing – 

-It is a dynamic routing algorithm in which each router computes a distance between itself and each possible destination i.e. its immediate neighbors.

-The router shares its knowledge about the whole network to its neighbors and accordingly updates the table based on its neighbors.

-The sharing of information with the neighbors takes place at regular intervals.

-It makes use of Bellman-Ford Algorithm for making routing tables.

Problems: 

– Count to infinity problem which can be solved by splitting horizon. 

– Good news spread fast and bad news spread slowly.

– Persistent looping problem i.e. loop will be there forever.


Link State Routing – 

-It is a dynamic routing algorithm in which each router shares knowledge of its neighbors with every other router in the network.

-A router sends its information about its neighbors only to all the routers through flooding.

-Information sharing takes place only whenever there is a change.

-It makes use of Dijkstra’s Algorithm for making routing tables.

Problems:

– Heavy traffic due to flooding of packets. 

– Flooding can result in infinite looping which can be solved by using the Time to live (TTL) field. 

![po-1](https://user-images.githubusercontent.com/80702342/216784099-025ff183-c36e-4a6e-9dea-814f36a972eb.png)
