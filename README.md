# Windows oriented client/server
Client/server application for directory events monitoring.

## Task
Create simple client-server application. 
### Server 
Server should monitor changes in some folder on the local hardware and log all events in the changelog. 
### Client
Client should catch all changes in the changelog and output data to the user. When run more then 1 client every instance should get all records from the changelog.

## Сriteria
Client server should be written in C++ lang in Microsoft Visual Studio without using additional libs. Allowed: Windows API, MFC, STL, boost
