# COMP 556 Project 3: Intra-Domain Routing Protocols

### Notes to graders
We are using `2` slip days for this project.

### Group members
* Haochen Zhang (jz118@rice.edu)
* Jinlin Li (jl288@rice.edu)
* Jiaqi He (jh166@rice.edu)
* Ye Zhou (yz202@rice.edu)

### Additional tests  
Four new tests are added to test both DV and LS implementation. Test outputs are included as `test{3,4,5,6}_{dv,ls}.out`. Detailed descriptions are listed in `test.txt` file.  
* `test3`: 4 nodes. Test `linkdying` and `linkcomingup` events
* `test4`: 3 nodes. Test `changedelay` events for poison reverse
* `test5`: 3 nodes. Test `changedelay`, `linkdying`, and `linkcomingup` events
* `test6`: 11 nodes. Test `changedelay`, `linkdying`, and `linkcomingup` events  

We have also implemented debug printing tools `printDVTable` (in `DistanceVector.cc`) and `printLSForwardingTable` (in `LinkState.cc`) to visualize our DV table and LS table and verify their correctness before and after sending packets. 