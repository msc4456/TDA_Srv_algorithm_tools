function [index_length,index_buffer] = Gen_indices_VisionSDK(xlength,ylength,gap) 
%与VISIONSDK中索引生成函数功能一致，为与3D view建立相同的纹理透明权重算法，并使用外部导入的lut
%此处将indices扩展为4个,设计上使用单一的vertex&uv lut

k=1;
x=0;
y=0;

while(y< ylength-gap)
% for y=0:gap:ylength -gap
    if y>0
        buffer(:,k) = y*xlength;k=k+1;
    end
    while(x < xlength)
        buffer(:,k) = y*xlength + x;k=k+1;
        buffer(:,k) = (y+gap)*xlength + x;k=k+1;
        x = x + gap;
    end
    x =0;
%     for x=0:gap:xlength
%         buffer(:,k) = y*xlength + x;k=k+1;
%         buffer(:,k) = (y+gap)*xlength + x;k=k+1;
%     end
    if y<ylength -1 -gap
        buffer(:,k) = (y+gap)*xlength + (xlength -1);k=k+1;
    end
    y = y +gap;
end

index_buffer =[buffer,buffer,buffer,buffer];        
index_length(1:4) = k -1;

% void generate_indices(t_index_buffer *index_buffer, unsigned int xlength, unsigned int ylength, unsigned int gap)
% {
% 	unsigned int *buffer = index_buffer->buffer;
% 	unsigned int x, y, k=0;
% 	for (y=0; y<ylength-gap; y+=gap)
% 	{
% 		if(y>0)
% 			buffer[k++]=(unsigned int) (y*xlength);
% 		for (x=0; x<xlength; x+=gap)
% 		{
% 			buffer[k++]=(unsigned int) (y*xlength + x);
% 			buffer[k++]=(unsigned int) ((y+gap)*xlength + x);
% 		}
% 		if(y < ylength - 1 - gap)
% 			buffer[k++]=(unsigned int) ((y+gap)*xlength + (xlength -1));
% 	}
% 	index_buffer->length = k;
% }