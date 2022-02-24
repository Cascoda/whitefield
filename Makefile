include config.inc

MAKE=make

ifneq ($(STACKLINE_RIOT),)
STACKLINE_DEP+=riot
STACKLINE_DEPCLEAN+=riot_clean
endif

ifneq ($(STACKLINE_CONTIKI),)
STACKLINE_DEP+=contiki
STACKLINE_DEPCLEAN+=contiki_clean
endif

ifneq ($(STACKLINE_CONTIKING),)
STACKLINE_DEP+=contiking
STACKLINE_DEPCLEAN+=contiking_clean
endif

ifneq ($(STACKLINE_OPENTHREAD),)
STACKLINE_DEP+=openthread
STACKLINE_DEPCLEAN+=openthread_clean
endif

ifneq ($(AIRLINE_NS3),)
AIRLINE_DEP+=ns3
AIRLINE_DEPCLEAN+=ns3_clean
endif


all: $(AIRLINE_DEP) whitefield $(STACKLINE_DEP)

whitefield:
	$(MAKE) -f src/commline/Makefile.commline all
	$(MAKE) -f src/utils/Makefile.utils all
	$(MAKE) -f src/airline/Makefile.airline all

riot:
	$(MAKE) -C $(STACKLINE_RIOT)/tests/whitefield

riot_clean:
	$(MAKE) -C $(STACKLINE_RIOT)/tests/whitefield clean

contiki:
	$(MAKE) -C $(STACKLINE_CONTIKI)/examples/ipv6/rpl-udp TARGET=whitefield

contiking:
	$(MAKE) -C $(STACKLINE_CONTIKING)/examples/rpl-udp TARGET=whitefield

contiki_clean:
	$(MAKE) -C $(STACKLINE_CONTIKI)/examples/ipv6/rpl-udp TARGET=whitefield clean

contiking_clean:
	$(MAKE) -C $(STACKLINE_CONTIKING)/examples/rpl-udp TARGET=whitefield clean

openthread:
	@if [ -d $(STACKLINE_OPENTHREAD) ]; then \
		cd $(STACKLINE_OPENTHREAD); \
		if [ ! -d build ]; then \
			echo "Bootstrapping OpenThread...";\
			./script/bootstrap; \
			./bootstrap; \
		fi; \
		./script/cmake-build simulation -DOT_PLATFORM=simulation -DOT_WHITEFIELD=ON -DOT_OTNS=ON -DOT_SIMULATION_VIRTUAL_TIME=ON -DOT_SIMULATION_VIRTUAL_TIME_UART=ON \
                                         -DOT_SIMULATION_MAX_NETWORK_SIZE=999 -DOT_COMMISSIONER=ON -DOT_JOINER=ON -DOT_BORDER_ROUTER=ON -DOT_SERVICE=ON -DOT_COAP=ON \
                                         -DOT_FULL_LOGS=OFF ; \
	fi

openthread_clean: 
	cd $(STACKLINE_OPENTHREAD); ./script/test clean

ns3:
	$(MAKE) -C $(AIRLINE_NS3)

ns3_clean:
	$(MAKE) -C $(AIRLINE_NS3) clean

clean: ns3_clean openthread_clean
	@rm -rf bin log pcap

allclean: $(STACKLINE_DEPCLEAN)
	$(MAKE) clean

tests:
	regression/regress.sh regression/full.set

