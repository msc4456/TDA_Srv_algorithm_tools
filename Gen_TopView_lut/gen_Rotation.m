%获取旋转平移矩阵齐次形式,和其逆变换形式
function [R,inv_R] = gen_Rotation( RRfin)
Matrix = RRfin(:,:,1);%RRfin第二维是第一维的重投优化
input1 = Matrix(:,1);
input2 = Matrix(:,2);
Transform = Matrix(:,3);
input3 = cross(input1, input2);%列向量1，2 叉乘获得Z的旋转分量
norm =[0,0,0,1];
R = [input1, input2, input3,Transform;norm];
inv_R = inv(R);