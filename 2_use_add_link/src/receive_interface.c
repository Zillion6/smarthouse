#include "receive_interface.h"
#include "smoke_interface.h"
#include "living_room_led_dev.h"
#include "bed_room_led_dev.h"
#include "fan_dev.h"
#include "beep_dev.h"
#include "lock_dev.h"
#include "myOLED.h"
#include "face.h"
#include "voice_interface.h"

struct gdevice *phead_dev = NULL;

int receive_interface_init()
{
    myOled_init();
    face_Init();

    phead_dev = add_living_room_led_dev_to_gdevice_list(phead_dev);
    phead_dev = add_bed_room_led_dev_to_gdevice_list(phead_dev);
    phead_dev = add_fan_dev_to_gdevice_list(phead_dev);
    phead_dev = add_beep_dev_to_gdevice_list(phead_dev);
    phead_dev = add_lock_dev_to_gdevice_list(phead_dev);

    return 0;
}

void receive_final()
{
    face_Finalize();
}

#if 0

typedef struct
{
    mqd_t mqd;
    struct  control *ctrl_phead;
}ctrl_info_t;

struct control
{
	char control_name[128]; //监听模块名称 
	int (*init)(void); //初始化函数
	void (*final)(void);//结束释放函数
	void *(*get)(void *arg);//监听函数，如语音监听
	void *(*set)(void *arg); //设置函数，如语音播报
	struct control *next;
};

struct gdevice
{
	char dev_name[128]; //设备名称
	int key; //key值，用于匹配控制指令的值
	int gpio_pin; //控制的gpio引脚
	int gpio_mode; //输入输出模式
	int gpio_status; //高低电平状态
	int check_face_status; //是否进行人脸检测状态
	int voice_set_status; //是否语音语音播报
	struct gdevice *next;
};

typedef struct
{
    int msg_len;
    unsigned char *buffer;
    ctrl_info_t *ctrl_info;
}recv_msg_t;

#endif

void* handler(void *arg)
{
    pthread_detach(pthread_self());

    recv_msg_t *recv_msg = NULL;
    struct gdevice *p_dev = NULL;
    struct gdevice *result_find_key_pdev = NULL;
    struct control *speaking_control = NULL;
    struct control *p_speaking_control = NULL;
    pthread_t tid;
    char *Open_or_Close = "Open";
    char *Success_or_Failure = "Success";
    int result_set_gpio_gdevice_status = -1;
    char *buf = (char *)malloc(sizeof(char) * 128);
    int smoke_flag = 0;
    double face = 0.00;

    p_dev = (struct gdevice *)malloc(sizeof(struct gdevice));
    result_find_key_pdev = (struct gdevice *)malloc(sizeof(struct gdevice));

    memset(buf, '\0', sizeof(buf));

    p_dev = phead_dev;

    if(arg != NULL)
    {
        recv_msg = (recv_msg_t *)arg;
//        printf("%s|%s|%d:msg_len = %d\n", __FILE__, __FUNCTION__, __LINE__, recv_msg->msg_len);
//        printf("%s|%s|%d:recv_msg->buffer:0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", __FILE__, __func__, __LINE__, recv_msg->buffer[0], recv_msg->buffer[1], recv_msg->buffer[2], recv_msg->buffer[3], recv_msg->buffer[4], recv_msg->buffer[5]);
    }

    if(recv_msg != NULL && recv_msg->buffer != NULL)
    {
        result_find_key_pdev = find_gdevice_by_key(p_dev, recv_msg->buffer[2]);
        
        if(result_find_key_pdev != NULL)
        {
            printf("%s|%s|%d:result_find_key_pdev->key = 0x%x\n", __FILE__, __func__, __LINE__, result_find_key_pdev->key);
//            printf("%s|%s|%d:recv_msg->buffer:0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", __FILE__, __func__, __LINE__, recv_msg->buffer[0], recv_msg->buffer[1], recv_msg->buffer[2], recv_msg->buffer[3], recv_msg->buffer[4], recv_msg->buffer[5]);

            result_find_key_pdev->gpio_status = (recv_msg->buffer[3] == 0 ? LOW : HIGH);

            if(result_find_key_pdev->check_face_status == 1)
            {
                face = alicloud_Search_Face();
                printf("%s|%s|%d: face = %f\n", __FILE__, __func__, __LINE__, face);
                
                if(face > 0.50)
                {
                    recv_msg->buffer[2] = 0x47;
                    result_set_gpio_gdevice_status = set_gpio_gdevice_status(result_find_key_pdev);

                    memset(buf, '\0', sizeof(buf));
                    buf = "Face recognition Successfully";
                }
                else
                {
                    recv_msg->buffer[2] = 0x46;

                    memset(buf, '\0', sizeof(buf));
                    buf = "Face recognition failure";
                }
            }

            else if(result_find_key_pdev->check_face_status == 0)
            {
                printf("%s|%s|%d:result_find_key_pdev->gpio_status = %d\n", __FILE__, __FUNCTION__, __LINE__, result_find_key_pdev->gpio_status);
                result_set_gpio_gdevice_status = set_gpio_gdevice_status(result_find_key_pdev);
                
                Open_or_Close = (recv_msg->buffer[3] == 0 ? "Open\n" : "Close\n");
                Success_or_Failure = (result_set_gpio_gdevice_status == 0? "Success" : "Failure");
                sprintf(buf, "%s %s %s", Open_or_Close, result_find_key_pdev->dev_name, Success_or_Failure);
            }

            if(result_find_key_pdev->voice_set_status == 1)
            {
                speaking_control = recv_msg->ctrl_info->ctrl_phead;
                p_speaking_control = speaking_control;

                while(p_speaking_control != NULL)
                {
                    if(!strcmp(p_speaking_control->control_name, "voice"))
                    {
                        if(recv_msg->buffer[2] == 0x45 && recv_msg->buffer[3] == 0)
                        {
                            smoke_flag = 1;
                        }
                        
                        printf("\033[45;30m%s|%s|%d:recv_msg->buffer:0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\033[0m\n", __FILE__, __FUNCTION__, __LINE__, recv_msg->buffer[0], recv_msg->buffer[1], recv_msg->buffer[2], recv_msg->buffer[3], recv_msg->buffer[4], recv_msg->buffer[5]);
                        printf("\033[44;30m%s|%s|%d:control_name = %s\033[0m\n", __FILE__, __FUNCTION__, __LINE__, p_speaking_control->control_name);
                        
                        pthread_create(&tid, NULL, (void *)p_speaking_control->set, (void *)recv_msg->buffer);
                        
                        break;
                    }

                    p_speaking_control = p_speaking_control->next;
                }
            }
            
            if(result_find_key_pdev->check_face_status == 1 && face > 0.5)
            {
                sleep(6);

                result_find_key_pdev->gpio_status = HIGH;
                set_gpio_gdevice_status(result_find_key_pdev);
            }

            if(smoke_flag == 1)
            {
                memset(buf, '\0', sizeof(buf));
                strcpy(buf, "FIRE ALARM!!!");
            }
            
            oled_show(buf);
        }
        
        else
        {
            printf("Device not found for key: 0x%x\n", recv_msg->buffer[2]);
        }
    }

//    printf("%s|%s|%d:phead_dev->key = 0x%x\n", __FILE__, __func__, __LINE__, phead_dev->key);
    
    pthread_exit(NULL);
}

void* receive_get(void *arg)
{
    printf("%s|%s|%d:\n", __FILE__, __FUNCTION__, __LINE__);
    pthread_detach(pthread_self());
    
    recv_msg_t *recv_msg = NULL;
    char *buf = NULL;
    struct mq_attr attr;
    mqd_t mqd = -1;
    int n_mq_receive = -1;
    pthread_t tid;

    if(arg != NULL)
    {   
        recv_msg = (recv_msg_t *)malloc(sizeof(recv_msg_t));
        recv_msg->ctrl_info = (ctrl_info_t *)arg;
        recv_msg->msg_len = -1;
        recv_msg->buffer = NULL;
    }

    if(recv_msg->ctrl_info != NULL)
    {
        mqd = recv_msg->ctrl_info->mqd;

        printf("%s|%s|%d:mqd = %d\n", __FILE__, __FUNCTION__, __LINE__, mqd);
    }

    if(mqd == -1)
    {
        pthread_exit(NULL);
    }

    if(mq_getattr(recv_msg->ctrl_info->mqd, &attr) == -1)
    {
        pthread_exit(NULL);
    }

    recv_msg->buffer = (unsigned char *)malloc(attr.mq_msgsize);
    buf = (unsigned char *)malloc(attr.mq_msgsize);

    if (!recv_msg->buffer || !buf) {
        perror("malloc");
        free(recv_msg->buffer);
        free(recv_msg);
        free(buf);
        pthread_exit(NULL);
    }

    printf("%s|%s|%d:\n", __FILE__, __FUNCTION__, __LINE__);

    memset(recv_msg->buffer, 0, attr.mq_msgsize);

    printf("%s|%s|%d:\n", __FILE__, __FUNCTION__, __LINE__);

    memset(buf, 0, attr.mq_msgsize);

    printf("%s|%s|%d:\n", __FILE__, __FUNCTION__, __LINE__);

    while(1)
    {
        n_mq_receive = mq_receive(mqd, buf, attr.mq_msgsize, NULL);
        
        printf("%s|%s|%d:\n", __FILE__, __FUNCTION__, __LINE__);

        if(n_mq_receive == -1)
        {
            perror("mq_receive");

            pthread_exit(NULL);
        }
        else if(buf[0] == 0xAA && buf[1] == 0x55 && buf[4] == 0x55 && buf[5] == 0xAA)
        {
            recv_msg->msg_len = n_mq_receive;
            memcpy(recv_msg->buffer, buf, n_mq_receive);
            
            printf("%s|%s|%d:recv_msg->buffer:0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", __FILE__, __func__, __LINE__, recv_msg->buffer[0], recv_msg->buffer[1], recv_msg->buffer[2], recv_msg->buffer[3], recv_msg->buffer[4], recv_msg->buffer[5]); 
            
            pthread_create(&tid, NULL, handler, (void *)recv_msg);
        }
    }

    pthread_exit(NULL);
}

struct control receive = 
{
    .control_name = "receive",
    .init = receive_interface_init,
    .final = receive_final,
    .get = receive_get,
    .set = NULL,
    .next = NULL
};

struct control* add_receive_to_ctrl_list(struct control* phead)
{
    return add_control_to_ctrl_list(phead, &receive);
//    printf("%s|%s|%d:\n", __FILE__, __FUNCTION__, __LINE__);
}