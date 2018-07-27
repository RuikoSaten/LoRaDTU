#include "Receiver.h"
#include "LoRa.h"
#include "ECC.h"
#include "LoRaUsart.h"
/**
 *  �����ߵ�������Ҫ�Ǹ�����ȡ�ײ㴮���жϽ��յ�������
 *  1.�����жϽ������ݻ��ں�������Ϊdma+�����ж�
 *  2.���ڵײ��ṩһ�� receiveEvent ��־λ
 *      ����־λ��λʱ�൱�ڽ��յ�һ�����������ݰ�
 *  3.���յ��������ݰ���Receiver ������ȡ���ݲ����� LoRa �ṩ�ķ�������
 *  4. LoRa ����������Ҫ����һ�� DataPacket ָ�� ����ڴ���malloc���벢�ƽ���LoRa
 *  5. LoRa ������һ�οռ�������  ����ռ�ͬ���� malloc ����
 *  6. �������ݿռ��� malloc ���� ʵ����Ч���ݴ��ݸ�Ӧ�ò� ������ݿռ������Ӧ�ò��ͷ�
 *  
 */ 
 
Receiver* receiver;
LinkedList* recvList;
DataPacket* receive(void);
/**
 *  ��ʱ���ݻ�����
 *
 *
 *
 */ 
uint8_t data[LoRaRxBufferSize];

void ReceiverInit(void){
    recvList = newLinkedList("recv");
    receiver = malloc(sizeof(Receiver));
    receiver->recvEvent = 0;
    receiver->RecvList = recvList;
    receiver->receive = receive;
}



/**
 *  �ú����ǹ��ϲ���û�ȡһ�����յ������ݰ���
 *  ��ȡ��ɺ󽫻�ӽ����б����Ƴ�������ݰ�
 *  @return DataPacket*   ���ݰ�ָ��
 *
 *
 */ 
DataPacket* receive(void){    
    return recvList->headRemove(recvList);
}



/**
 *  ������������ش����ж��ṩ�Ľ�������¼���־λ
 *  �����ڽ��յ�һ֡���ݵ�ʱ���ȥ��ȡһ֡���ݲ�������ת�Ƶ���ʱ����data
 *  Ȼ���� LoRa �ṩ�� unPacket �������
 *
 */ 
void _receive(void){
    if(LoRaRXEvent){
        uint8_t* buffer = NULL;
        
        if(using_LoRaRxBuffer1){
            buffer = LoRaRxBuffer2;
        }else{
            buffer = LoRaRxBuffer1;
        }
        
        memset(data,0,sizeof(data));                //�����ʱ������
        //memcpy(data,buffer,receiveLength);          //�������ݵ���ʱ������
        
 /***************************************************************************/
        //���ܸ���ת���ַ�
        uint8_t ch,*pdata = data;
        for(int8_t i = 0,j = 0;i < receiveLength;i++,j++){
            ch = *(buffer+i+1);
            if(*(buffer+i) == 0x1B &&(ch == 0x1B || ch == 0x04 || ch == 0x01)){
                i++;
            }
            *(pdata + j) = *(buffer+i);
        }
 /***************************************************************************/       
            
        memset(receiveBuffer,0,LoRaRxBufferSize*sizeof(uint8_t));//��մ��ڽ��ջ�����
        DataPacket* packet= malloc(sizeof(DataPacket));
        int8_t flag = unPacket(packet,data,receiveLength);
        if(flag == 0 || flag == -1){
            free(packet);
            receiveEvent = 0;
            return ;
        }
        
        if(recvList->size >= RECV_MAX_LEN){
            recvList->headRemove(recvList);
        }
        
        
        if(flag == 1){      //�����Ǹ�������  ����������������Ϊ0������
          
            #if DEBUG
            printf("LORA ���յ����ݰ� crcΪ %x\r\n",packet->crc);
            printf("LORA ���յ����ݰ� lenΪ %x\r\n",packet->dataBytes.length);
            #endif
            
            
            if(packet->dataBytes.length <= 2){
                ECC->remove(packet->dataBytes.data[packet->dataBytes.length-1]);
                free(packet->dataBytes.data);
                free(packet);
                //����·�ɱ�
            }else{
                recvList->tailInsert(recvList,packet);    //�����ݼ��뵽�����б������������ɵ�ʱ��free��Դ
                Sender->sendAck(packet->source,packet->crc);   //ֱ�ӷ���ȷ�ϰ�
            }
            
        }else if(flag == 2){//���ݲ��Ǹ�������
            if(packet->count == 0){
                free(packet->dataBytes.data);
                packet->dataBytes.data = malloc(sizeof(uint8_t)*2);
                packet->dataBytes.length = 2;
                packet->dataBytes.data[0] = 0xff;
                packet->dataBytes.data[1] = packet->crc;
                packet->destination = packet->source;
                packet->source = localhost;
                packet->count = 0x10;
            }
            Sender->send(packet);   
            
        }
        LoRaRXEvent = 0;
    }
    
    
}







