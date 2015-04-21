#include <curl/curl.h>
#include <stdio.h>
#include <sys/stat.h>

#include <sys/fcntl.h>

#include <string.h>

CURL *curl;
CURLcode res;

int fd;


size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	printf("test.....");
	bzero(stream,1024);
	// if (strlen((char *)stream) + strlen((char *)ptr) > 999999) 
	// 	return 0;
	strcat(stream, (char *)ptr);
	write(fd,stream,strlen(stream));
	printf("size=%ld,nmemb = %ld, stream = %s\n",size,nmemb,stream);
	return size*nmemb;
}

char *down_file(char *filename)
{
	static char str[1024];
	strcpy(str, "");
	curl_easy_setopt(curl, CURLOPT_URL, filename); //设置下载地址

	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);//设置超时时间

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);//设置写数据的函数

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, str);//设置写数据的变量

	res = curl_easy_perform(curl);//执行下载

	str[1023] = '\0';
	if(CURLE_OK != res) return NULL;//判断是否下载成功

	return str;
}

int main()
{
	char url[200];
	curl = curl_easy_init();//对curl进行初始化

	 int ret;
    fd = open("./test.mp3",O_WRONLY|O_CREAT,0644);


	char *result;
	while(fgets(url, 200, stdin)){
		result = down_file(url);
		// if (result) 
		// 	// puts(result);
		// else 
		// 	puts("Get Error!");
		printf("\nPlease Input a url:");
	}
	curl_easy_cleanup(curl);//释放curl资源
	close(fd);

	return 0;
}