/*
 *  This file is a part of the PARUS project.
 *  Copyright (C) 2006  Alexey N. Salnikov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Alexey N. Salnikov (salnikov@cmc.msu.ru)
 *
 */

/*
 *****************************************************************
 *                                                               *
 * This file is one of the parus source code files. This file    *
 * written by Alexey Salnikov and will be modified by            *
 * Vera Goritskaya                                               *
 * Ivan Beloborodov                                              *
 * Andreev Dmitry                                                *
 *                                                               *
 *****************************************************************
 */

#include <mpi.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <dlfcn.h>


#ifdef _GNU_SOURCE
#include <getopt.h>
#else
#include <unistd.h>
#endif

#include "my_time.h"
#include "my_malloc.h"
#include "parus_config.h"

#include "string_id_converters.h"
#include "data_write_operations.h"
#include "network_test2.h"
#include "types.h"
#include "easy_matrices.h"
#include "tests_common.h"
#include "parse_arguments.h"

int comm_size; //константа отвещающая за число процесов задействованных в программе
int comm_rank; // константа отвещающая за номер процесса 



int main(int argc,char **argv)
{
    MPI_Status status; //структура, содержащая следующие поля: MPI_SOURCE (источник), MPI_TAG (метка), MPI_ERROR (ошибка). определяет статус сообщения 

    Test_time_result_type *times=NULL; /* old px_my_time_type *times=NULL;*/ //отвечает за время работы программы
     
    /*
     * The structure with network_test parameters.
     */
    struct network_test_parameters_struct test_parameters;
    struct network_test_parametrs_of_types types_parameters;
    /*
     * NetCDF file_id for:
     *  average
     *  median
     *  diviation
     *  minimal values
     *
     */
    int netcdf_file_av;
    int netcdf_file_me;
    int netcdf_file_di;
    int netcdf_file_mi;

    /*
     * NetCDF var_id for:
     *  average
     *  median
     *  diviation
     *  minimal values
     *
     */
    int netcdf_var_av;
    int netcdf_var_me;
    int netcdf_var_di;
    int netcdf_var_mi;

    /*
     * Variables to concentrate test results
     *
     * This is not C++ class but very like.
     */
    Easy_matrix mtr_av;
    Easy_matrix mtr_me;
    Easy_matrix mtr_di;
    Easy_matrix mtr_mi;


    char test_type_name[100]; // имя теста 
    int i,j; // положение в матрице


    char** host_names=NULL; 
    char host_name[256];


    int flag;
	
	/*
    int help_flag = 0;
    int version_flag = 0;
	*/
    int error_flag = 0;
	/*
    int ignore_flag = 0;
    int median_flag = 0;
    int deviation_flag = 0;
    */


    int tmp_mes_size; // длина сообщения ?

    /*Variables for MPI struct datatype creating*/
    MPI_Datatype struct_types[4]= {MPI_DOUBLE,MPI_DOUBLE,MPI_DOUBLE,MPI_DOUBLE};
    MPI_Datatype MPI_My_time_struct; 
    int blocklength[4]= {1,1,1,1/*,1*/};
    MPI_Aint displace[4],base;

    int step_num=0;

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD,&comm_rank);
    if(comm_rank == 0) // 
    {
        if ( comm_size == 1 ) // бессмысленное использование  программы
         {
            error_flag = 1;
            printf( "\n\nYou tries to run this programm for one MPI thread!\n\n" );
        }

        if(parse_network_test_arguments(argc,argv,&test_parameters)) // идем парсить нашу строку определяет \
        type \
        file \
        num_iterations\
        begin \
        end   \
        step \
        length_noise_message  \
        num_noise_message   \
        progs_noise \
        version \
        help   \
        resume \
        ignore 
        {
            MPI_Abort(MPI_COMM_WORLD,-1);
            return -1;
        }

        host_names = (char**)malloc(sizeof(char*)*comm_size);  //  не знаю пока что это ??? скорее всего названия тестов или имена файлов , первое скорее всего
        if(host_names==NULL)
        {
            printf("Can't allocate memory %d bytes for host_names\n",(int)(sizeof(char*)*comm_size));
            MPI_Abort(MPI_COMM_WORLD,-1);
            return -1;
        }

        for ( i = 0; i < comm_size; i++ )
        {
            host_names[i] = (char*)malloc(256*sizeof(char));
            if(host_names[i]==NULL)
            {
                printf("Can't allocate memory for name proc %d\n",i);
                MPI_Abort(MPI_COMM_WORLD,-1);
            }
        }
    } /* End if(rank==0) */

    /*
     * Going to get and write all processors' hostnames // какая-то дичь по записи именов всех имен сообщение   и обработка по одному 
     */
    gethostname( host_name, 255 );

    if ( comm_rank == 0 )
    {
        for ( i = 1; i < comm_size; i++ )
			MPI_Recv( host_names[i], 256, MPI_CHAR, i, 200, MPI_COMM_WORLD, &status );
        strcpy(host_names[0],host_name);
		
    }
    else
    {
        MPI_Send( host_name, 256, MPI_CHAR, 0, 200, MPI_COMM_WORLD );
    }

    /*
     * Initializing num_procs parameter
     */
    test_parameters.num_procs=comm_size;

    if( comm_rank == 0)
    {
        /*
         *
         * Matrices initialization
         *
         */
        flag = easy_mtr_create(&mtr_av,0,comm_size); //создаем матрицу 
        if( flag==-1 )
        {
            printf("Can not to create average matrix to story the test results\n");
            MPI_Abort(MPI_COMM_WORLD,-1);
            return -1;
        }
        flag = easy_mtr_create(&mtr_me,0,comm_size);
        if( flag==-1 )
        {
            printf("Can not to create median matrix to story the test results\n");
            MPI_Abort(MPI_COMM_WORLD,-1);
            return -1;
        }
        flag = easy_mtr_create(&mtr_di,0,comm_size);
        if( flag==-1 )
        {
            printf("Can not to create deviation matrix to story the test results\n");
            MPI_Abort(MPI_COMM_WORLD,-1);
            return -1;
        }
        flag = easy_mtr_create(&mtr_mi,0,comm_size);
        if( flag==-1 )
        {
            printf("Can not to create min values matrix to story  the test results\n");
            MPI_Abort(MPI_COMM_WORLD,-1);
            return -1;
        }
        
        if(create_netcdf_header(AVERAGE_NETWORK_TEST_DATATYPE,&test_parameters,&netcdf_file_av,&netcdf_var_av))
        {
            printf("Can not to create file with name \"%s_average.nc\"\n",test_parameters.file_name_prefix);
            MPI_Abort(MPI_COMM_WORLD,-1);
            return -1;
        }

        if(create_netcdf_header(MEDIAN_NETWORK_TEST_DATATYPE,&test_parameters,&netcdf_file_me,&netcdf_var_me))
        {
            printf("Can not to create file with name \"%s_median.nc\"\n",test_parameters.file_name_prefix);
            MPI_Abort(MPI_COMM_WORLD,-1);
            return -1;
        }


        if(create_netcdf_header(DEVIATION_NETWORK_TEST_DATATYPE,&test_parameters,&netcdf_file_di,&netcdf_var_di))
        {
            printf("Can not to create file with name \"%s_deviation.nc\"\n",test_parameters.file_name_prefix);
            MPI_Abort(MPI_COMM_WORLD,-1);
            return -1;
        }

        if(create_netcdf_header(MIN_NETWORK_TEST_DATATYPE,&test_parameters,&netcdf_file_mi,&netcdf_var_mi))
        {
            printf("Can not to create file with name \"%s_min.nc\"\n",test_parameters.file_name_prefix);
            MPI_Abort(MPI_COMM_WORLD,-1);
            return -1;
        }

	
	if(create_test_hosts_file(&test_parameters,host_names))		
	{
		printf("Can not to create file with name \"%s_hosts.txt\"\n",test_parameters.file_name_prefix);
    		MPI_Abort(MPI_COMM_WORLD,-1);
    		return -1;
	}

        /*
         *
         * Printing initial message for user
         *
         */
        printf("network test (%d processes):\n\n", comm_size);
        get_test_type_name(test_parameters.test_type,test_type_name);
        printf("\ttest type\t\t\t\"%s\"\n",test_type_name);
        printf("\tbegin message length\t\t%d\n",test_parameters.begin_message_length);
        printf("\tend message length\t\t%d\n",test_parameters.end_message_length);
        printf("\tstep length\t\t\t%d\n",test_parameters.step_length);
        printf("\tnoise message length\t\t%d\n",test_parameters.noise_message_length);
        printf("\tnumber of noise messages\t%d\n",test_parameters.num_noise_messages);
        printf("\tnumber of noise processes\t%d\n",test_parameters.num_noise_procs);
        printf("\tnumber of repeates\t\t%d\n",test_parameters.num_repeats);
	printf("\tdependence of noise processes\t%d\n",test_parameters.dep_noise_procs);
	printf("\tstep degree\t\t\t%d\n",test_parameters.pow_step_length);
        printf("\tresult file average\t\t\"%s_average.nc\"\n",test_parameters.file_name_prefix);
        printf("\tresult file median\t\t\"%s_median.nc\"\n",test_parameters.file_name_prefix);
        printf("\tresult file deviation\t\t\"%s_deviation.nc\"\n",test_parameters.file_name_prefix);
        printf("\tresult file minimum\t\t\"%s_min.nc\"\n",test_parameters.file_name_prefix);
	printf("\tresult file hosts\t\t\"%s_hosts.txt\"\n\n",test_parameters.file_name_prefix);


    } /* End preparation (only in MPI process with rank 0) */

    /*
     * Broadcasting command line parametrs
     *
     * The structure network_test_parameters_struct contains 9
     * parametes those are placed at begin of structure.
     * So, we capable to think that it is an array on integers.
     *
     * Little hack from Alexey Salnikov.
     */
    MPI_Bcast(&test_parameters,9,MPI_INT,0,MPI_COMM_WORLD);


    /*
     * Creating struct time type for MPI operations
     */
    {
        Test_time_result_type tmp_time;
        MPI_Address( &(tmp_time.average), &base);
        MPI_Address( &(tmp_time.median), &displace[1]);
        MPI_Address( &(tmp_time.deviation), &displace[2]);
        MPI_Address( &(tmp_time.min), &displace[3]);
    }
    displace[0]=0;
    displace[1]-=base;
    displace[2]-=base;
    displace[3]-=base;
    MPI_Type_struct(4,blocklength,displace,struct_types,&MPI_My_time_struct);
    MPI_Type_commit(&MPI_My_time_struct);


    times=(Test_time_result_type* )malloc(comm_size*sizeof(Test_time_result_type));
    if(times==NULL)
    {
		printf("Memory allocation error\n");
		MPI_Abort(MPI_COMM_WORLD,-1);
		return -1;
    }

    MPI_Barrier(MPI_COMM_WORLD); //синхронизация процессов 


    /*
     * Circle by length of messages
     */


    for
	    (
	     tmp_mes_size=test_parameters.begin_message_length;
	     tmp_mes_size<test_parameters.end_message_length;
	     step_num++
	     )
	     // изменение длины сообщения нелинейно менять здесь,\
	      надо узнать по поводу других способов изменения \
	      кроме линейного , дело получаса менять  
    {
  	    void *library_handler;
	    int (*test)(struct network_test_parametrs_of_types types_parameters);
	    char *adress=NULL; 
	    get_test_type_name(test_parameters.test_type,test_type_name);
	    adress = (char*)malloc(sizeof(char)*(strlen(getenv("PWD"))+13+strlen(test_type_name)));
	    strcpy(adress,getenv("PWD"));
	    strcat(adress,"/lib/lib");    
	    strcat(adress,test_type_name);
	    strcat(adress,"2.so");
	    library_handler =dlopen(adress,RTLD_LAZY || RTLD_GLOBAL);
	    types_parameters.times=times;
	    types_parameters.mes_length=tmp_mes_size;
	    types_parameters.num_repeats=test_parameters.num_repeats;
	    types_parameters.num_noise_repeats=test_parameters.num_noise_messages;
	    types_parameters.noise_message_length=test_parameters.noise_message_length;
	    types_parameters.num_noise_procs=test_parameters.num_noise_procs;
	    types_parameters.dep_noise_procs=test_parameters.dep_noise_procs;
	
	    if(!library_handler)
	    {
		//если ошибка, то вывести ее на экран
		fprintf(stderr,"dlopen() error: %s\n", dlerror());
		exit(1); // в случае ошибки можно, например, закончить работу программы
	    }
	    test = dlsym(library_handler,test_type_name);
            (*test)(types_parameters);	
	    dlclose(library_handler);
	    free(adress);



		// по сути у нас конец выполнения теста и далее идет
        MPI_Barrier(MPI_COMM_WORLD); //синхронизация процессов



        if(comm_rank==0) //  если процесс главный ?
        {
            
            for(i=0; i<comm_size; i++) // теперь бегаем по всей остальной матрице 
            {
          
                if (i!=0)
                	MPI_Recv(times,comm_size,MPI_My_time_struct,i,100,MPI_COMM_WORLD,&status); // получаем сообщения о том что строки готовы? , немного не понимаю 
                for(j=0; j<comm_size; j++)
                {
                
					
                	MATRIX_FILL_ELEMENT(mtr_av,j,times[j].average);
               	 	MATRIX_FILL_ELEMENT(mtr_me,j,times[j].median);
                	MATRIX_FILL_ELEMENT(mtr_di,j,times[j].deviation);
                	MATRIX_FILL_ELEMENT(mtr_mi,j,times[j].min);
                }
                
                
                
            	if(netcdf_write_matrix(netcdf_file_av,netcdf_var_av,step_num,i,mtr_av.sizey,mtr_av.body))
            	{
             	   	printf("Can't write average matrix to file.\n");
            	    MPI_Abort(MPI_COMM_WORLD,-1);
                	return 1;
         	   	}

            	if(netcdf_write_matrix(netcdf_file_me,netcdf_var_me,step_num,i,mtr_me.sizey,mtr_me.body))
            	{
            	    printf("Can't write median matrix to file.\n");
            	    MPI_Abort(MPI_COMM_WORLD,-1);
            	    return 1;
            	}

            	if(netcdf_write_matrix(netcdf_file_di,netcdf_var_di,step_num,i,mtr_di.sizey,mtr_di.body))
            	{
            	    printf("Can't write deviation matrix to file.\n");
            	    MPI_Abort(MPI_COMM_WORLD,-1);
            	    return 1;
            	}

            	if(netcdf_write_matrix(netcdf_file_mi,netcdf_var_mi,step_num,i,mtr_mi.sizey,mtr_mi.body))
            	{
            	    printf("Can't write  matrix with minimal values to file.\n");
            	    MPI_Abort(MPI_COMM_WORLD,-1);
            	    return 1;
            	}
            	
            }
			

            printf("message length %d finished\r",tmp_mes_size);
            fflush(stdout);
        } 
		/* end comm rank 0 */
        else
        {
            MPI_Send(times,comm_size,MPI_My_time_struct,0,100,MPI_COMM_WORLD);
        }
        /* end for cycle .
         * Now we  go to the next length of message that is used in
         * the test perfomed on multiprocessor.
         */
	if (test_parameters.pow_step_length<=1)
         tmp_mes_size+=test_parameters.step_length;
	else
	tmp_mes_size=test_parameters.begin_message_length+pow((double)test_parameters.step_length,(double)(test_parameters.pow_step_length*(step_num+1)));
    }
    
    /* TODO
     * Now free times array.
     * It should be changed in future for memory be allocated only once.
     *
     * Times array should be moved from return value to the input argument
     * for any network_test.
     */

	free(times);


	if(comm_rank==0)
    {

        netcdf_close_file(netcdf_file_av);
        netcdf_close_file(netcdf_file_me);
        netcdf_close_file(netcdf_file_di);
        netcdf_close_file(netcdf_file_mi);

        for(i=0; i<comm_size; i++)
        {
            free(host_names[i]);
        }
        free(host_names);

        free(mtr_av.body);
        free(mtr_me.body);
        free(mtr_di.body);
        free(mtr_mi.body);

        printf("\nTest is done\n");
    }
    MPI_Finalize();
    return 0;
} /* main finished */

