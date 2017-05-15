#
# This file is a part of the PARUS project and  makes the core of the parus system
# Copyright (C) 2006  Alexey N. Salnikov
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# Alexey N. Salnikov (salnikov@cmc.msu.ru)
#

#
# This file makes the nework_test of the parus system
#

include ../../config


#LD_LIBRARY_PATH = ${PWD}
#LDFLAGS = -L${LD_LIBRARY_PATH}

FILES_COMMON=\
	tests_common.o

FILES_TEST2=\
	network_test2.o\
	test_noise_common.o\
	get2.o\
	parse_arguments.o\
	put2.o\
	all_to_all2.o\
	async_one_to_one2.o\
	one_to_one2.o\
	send_recv_and_recv_send2.o\
	noise2.o\
	noise_blocking2.o\
	bcast2.o
	

FILES_SO=\
	get2.so\
	all_to_all2.so\
	async_one_to_one2.so\
	one_to_one2.so\
	send_recv_and_recv_send2.so\
	noise2.so\
	noise_blocking2.so\
	bcast2.so\
	put2.so


FILES_PRINT_FROM_TO=\
	print_from_to.o

EXECS= print_from_to convert_to_netcdf network_test2

all: create_lib $(EXECS)

network_test2: $(FILES_TEST2)  $(FILES_COMMON) $(FILES_SO)  
	$(MPI_CCLINKER) $(MPI_CCLINKER_FLAGS) $(MPI_LIB_PATH) $(NETCDF_INCLUDE) -L ../core -o network_test2 $(FILES_COMMON) $(FILES_TEST2) $(MPI_LIBS) -lparus_network  -lnetcdf -lm -ldl


print_from_to: $(FILES_PRINT_FROM_TO)
	$(CXXLINKER) $(CXXLINKER_FLAGS) $(MPI_LIB_PATH) $(NETCDF_INCLUDE) -L ../core -o print_from_to $(FILES_PRINT_FROM_TO) $(MPI_LIBS) -lnetcdf

convert_to_netcdf: convert_to_netcdf.o $(FILES_COMMON)
	$(MPI_CXXLINKER) $(MPI_CXXLINKER_FLAGS) $(MPI_LIB_PATH) -L ../core -o  convert_to_netcdf convert_to_netcdf.o ../core/string_id_converters.o  $(FILES_COMMON) $(MPI_LIBS) -lparus_network -lnetcdf


clean:
	rm  -f ./*.o
	rm  -f ./*.so
	rm  -f $(EXECS)


install: all
	cp -rf $(EXECS) $(INSTALL_DIR)/bin/
	cp -rf $(PWD)/lib $(INSTALL_DIR)/bin/

create_lib: 
	mkdir -p $(PWD)/lib


%.so: %.o
	$(MPI_CC) -shared -rdynamic  -I ../core  -I ../.. -o ./lib/lib$@ $^ $(FILES_COMMON) ../core/my_time.o ../core/my_malloc.o \
network_test2.o test_noise_common.o

%.o: %.c
	$(MPI_CC) $(MPI_CC_FLAGS) $(MPI_CC_INCLUDE) -fPIC -rdynamic  -L ../core  -I ../core  -I ../.. -c $^ -o $@


%.o: %.cpp
	$(MPI_CXX) $(MPI_CXX_FLAGS) $(MPI_CXX_INCLUDE) -I ../core -I ../.. -c $^ -o $@
