#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

unsigned __int64 int_from_bytes(unsigned char *list, unsigned int start, unsigned int end){
	if(start > end){printf("��Ʋ����� ����Ʈ ��ȯ ����\n");return 1;}
	unsigned int i, j, length;
	length = end - start;
	unsigned __int64 convert[length];
	unsigned __int64 result=0;
	result = 0;
	for(i=start,j=0; i<=end; i++,j++){
		convert[j] = list[i] & 0xFF;
		result += (convert[j] << (j*8));
	}
	return result;
}

unsigned int* clusterrun_from_bytes(unsigned char value){
	static unsigned int convert[2];
	convert[0] = (value & 0xF0) >> 4;
	convert[1] = value & 0x0F;
	return convert;
}

void main() {	
	FILE *drive_file, *mft_file, *result_file;
	unsigned char ref[0x200];		//���� �����
	unsigned char mft[0x1000];		//MFT ����� 
	char file_str[7] = "\\\\.\\";
	char *arg = malloc(sizeof(char) * 2);
	int i;
	
	system("color 0b");
	printf("\n***MFTparser***\n\n\n$MFT �����ϰ� ���� ����̺긦 �Է����ּ���\n");
	printf("\nINFO: C:�Ǵ� D:����̺꿡�� �����Ϸ��� ������ �������� �������ּ���\n\nex) C:\n");
	printf("\n����̺� �̸�: ");
	scanf("%s",arg);
	while(strlen(arg) != 2){
		printf("�߸��Է��Ͽ����ϴ�\n");
		printf("ex) C:, D:, E:, F:, G:, H: ���\n���� ���� �������� �Է����ּ���\n");
		printf("\n����̺� �̸�: ");
		scanf("%s",arg);}
	strcat(file_str, arg);
	
	drive_file = fopen(file_str, "rb");
	if (drive_file == NULL){
		printf("���� ���� ����");
		getch();exit(1);}
	
	mft_file = fopen("$MFT", "wb");
	if (mft_file == NULL){
		printf("���� ���� ����");
		getch();exit(1);}
	
	result_file = fopen("INFO.txt", "w");
	if (result_file == NULL){
		printf("���� ���� ����");
		getch();exit(1);}
	
	//�⺻ ���� ���� 
	fread(ref, sizeof(ref), 1, drive_file);
	unsigned __int64 mft_location = int_from_bytes(ref, 0x30, 0x37);
	unsigned int BPS = int_from_bytes(ref, 0x0b, 0x0c);
	unsigned int SPC = int_from_bytes(ref, 0x0D, 0x0D);
	fprintf(result_file,"MFTparser summary");
	fprintf(result_file,"\nLogical Cluster Number for the file $MFT: 0x");
	fprintf(result_file,"%x ",mft_location);
	fprintf(result_file,"\nBytes Per Sector: 0x");
	fprintf(result_file,"%x ",BPS);
	fprintf(result_file,"\nSectors Per Cluster: 0x");
	fprintf(result_file,"%x ",SPC);
	mft_location = mft_location * BPS * SPC;
	fprintf(result_file,"MFT address: 0x%llx\n\n", mft_location);
	
	
	//MFT ��ü ���� ����
	_fseeki64(drive_file, mft_location, SEEK_SET);
	fread(ref, sizeof(ref), 1, drive_file);
	unsigned int *cluster_run_def;
	unsigned __int64 cluster_run_start[2]={0,};
	unsigned __int64 cluster_run_count[2]={0,};
	unsigned int cluster_extend = 0; 	//Ŭ�����ͷ��� 2������ ���θ� üũ 
	cluster_run_def = clusterrun_from_bytes(ref[0x140]);
	cluster_run_start[0] = int_from_bytes(ref, 0x140+1+cluster_run_def[1], 0x140+cluster_run_def[1]+cluster_run_def[0]);
	cluster_run_count[0] = int_from_bytes(ref, 0x140+1, 0x140+cluster_run_def[1]);
	cluster_run_start[0] = cluster_run_start[0] * BPS * SPC; 			//Ŭ������ ������� ����� 
	cluster_run_count[0] = cluster_run_count[0] * BPS * SPC; 			//1Ŭ������(0x1000)�� ��Ʈ��(0x400)4��  
	fprintf(result_file,"MFT entry cluster run: 0x%llx\n", cluster_run_start[0]);
	fprintf(result_file,"MFT entry cluster count: 0x%llx\n", cluster_run_count[0]);

	if(ref[0x140+cluster_run_def[1]+cluster_run_def[0] + 1] != 0x00){
		cluster_extend = 1;
		unsigned int cluster_offset = 0x140+cluster_run_def[1]+cluster_run_def[0] + 1;
		cluster_run_def = clusterrun_from_bytes(ref[cluster_offset]);
		cluster_run_start[1] = int_from_bytes(ref, cluster_offset+1+cluster_run_def[1], cluster_offset+cluster_run_def[1]+cluster_run_def[0]);
		cluster_run_count[1] = int_from_bytes(ref, cluster_offset+1, cluster_offset+cluster_run_def[1]);
		cluster_run_start[1] = cluster_run_start[0]+(cluster_run_start[1] * BPS * SPC); //Ŭ������ ������� ����� 
		cluster_run_count[1] = cluster_run_count[1] * BPS * SPC; 						//1Ŭ������(0x1000)�� ��Ʈ��(0x400)4��  
		fprintf(result_file,"MFT entry cluster run2: 0x%llx\n", cluster_run_start[1]);
		fprintf(result_file,"MFT entry cluster count2: 0x%llx\n", cluster_run_count[1]);
	}
	printf("\n���� ���� �ۼ� �Ϸ�\n\n");
	fclose(result_file);
	
	//MFT����  
	unsigned __int64 count;
	float progress = 0;
	float total[2];
	total[0] = cluster_run_count[0]/(BPS*SPC);
	total[1] = cluster_run_count[1]/(BPS*SPC);

	_fseeki64(drive_file, cluster_run_start[0], SEEK_SET);
	for(count=0; count<total[0]; count++){
		fread(mft, sizeof(mft), 1, drive_file);
		fwrite(mft, sizeof(mft), 1, mft_file);
		if(cluster_extend == 0)
			progress += (1 / total[0] * 100);
		else if(cluster_extend == 1)
			progress += (1 / (total[0]+total[1]) * 100);
		printf("$MFT ���� ������...%0.2f%%\r",progress);
	}
	if(cluster_extend == 1){								//2��° Ŭ������ ���� �ִٸ�, 
		_fseeki64(drive_file, cluster_run_start[1], SEEK_SET);
		for(count=0; count<total[1]; count++){
			fread(mft, sizeof(mft), 1, drive_file);
			fwrite(mft, sizeof(mft), 1, mft_file);
			progress += (1 / (total[0]+total[1]) * 100);
			printf("$MFT ���� ������...%0.2f%%\r",progress);
		}
	}

	printf("$MFT ���� ������...100.0%%\n");
	_fseeki64(mft_file, 0, SEEK_END);
	//int byte_unit=0;
	//unsigned __int64 file_size = _ftelli64;
	//while((flie_size / 0x400) != 0){
	//	file_size /= 0x400
	//	byte_unit++}
		
	printf("\n$MFT ���� (%lldbytes)\n", _ftelli64(mft_file)); 
	fclose(drive_file);
	fclose(mft_file);
	
	//�м����α׷� ����
	char *check = malloc(sizeof(char) * 1); 
	printf("\n������ $MFT�� �м��ϽǷ���?(y/n) ");
	scanf("%s",check);
	while(check[0]!='y'&&check[0]!='Y'&&check[0]!='n'&&check[0]!='N'||strlen(check)>2){
		printf("�߸��Է��ϼ̽��ϴ�\n");printf("������ $MFT�� �м��ϽǷ���?(y/n) ");
		scanf("%s",check);
	}
	if(check[0]=='y'||check[0]=='Y'){
		printf("\n������ Ŭ ��� �ð��� �ټ� �ɸ� �� �־��...");
		system("analyzeMFT\\analyzeMFT.exe -f $MFT -o analyzed_MFT.csv -p");
		printf("\n���� Finder.exe�� ������ ã�ƺ�����!!!\n");
		printf("\n\nContacts chanbining@gmail.com\n");getch();}
	else if(check[0]=='n'||check[0]=='N'){
		printf("\n\nContacts chanbining@gmail.com\n");getch();exit(0);}
}
