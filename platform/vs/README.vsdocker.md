HOWTO Use Virtual Switch (Docker)


1. Create a docker with two front panel ports

```
$ docker run -id --name sw debian bash
$ sudo ./create_vnet.sh -n 2 sw
$ ip netns list
sw-srv1 (id: 6)
sw-srv0 (id: 5)
```

2. Start sonic virtual switch docker

```
$ docker run --privileged --network container:sw --name vs -d docker-sonic-vs
```

3. Setup IP in the virtual switch docker

```
$ docker exec -it vs bash
root@2e9b5c2dc2a2:/# config interface ip add Ethernet0 10.0.0.0/31
root@2e9b5c2dc2a2:/# config interface ip add Ethernet4 10.0.0.2/31
root@2e9b5c2dc2a2:/# config interface startup Ethernet0
root@2e9b5c2dc2a2:/# config interface startup Ethernet4
```

4. Setup IP in the server network namespace

```
$ sudo ip netns exec sw-srv0 ifconfig eth0 10.0.0.1/31
$ sudo ip netns exec sw-srv0 ip route add default via 10.0.0.0
$ sudo ip netns exec sw-srv1 ifconfig eth0 10.0.0.3/31
$ sudo ip netns exec sw-srv1 ip route add default via 10.0.0.2
```

5. Ping from sw-srv0 to sw-srv1

```
$ sudo ip netns exec sw-srv0 ping  10.0.0.3
PING 10.0.0.3 (10.0.0.3) 56(84) bytes of data.
64 bytes from 10.0.0.3: icmp_seq=1 ttl=63 time=0.137 ms
64 bytes from 10.0.0.3: icmp_seq=2 ttl=63 time=0.148 ms
64 bytes from 10.0.0.3: icmp_seq=3 ttl=63 time=0.149 ms
```
