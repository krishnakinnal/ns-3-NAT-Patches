#include<iostream>
#include"ns3/internet-module.h"
using namespace ns3;

int main()
{
std::cout<<"hello"<<"\n";
uint8_t x=17;
if(x==IPPROTO_UDP)
std::cout<<"hello again"<<"\n";

return 0;
}

