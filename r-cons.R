library(Rcpp)
setwd("~/tmp/joycons/")
module = Rcpp::sourceCpp("r-joycons-udpread.cpp")
module

for(i in 1:200) { x <- udp_message(); cat(x) }

time1 = system.time( m <- plyr::ldply(lapply(1:700 , function(i) { read.table(textConnection(udp_message() ))  } )  )  )

#cal = apply(m,2,mean)

#earth = mean(mag(m[,1],m[,2],m[,3]))

plot( mag(m[,1],m[,2],m[,3]) , t="l",ylim=c(0,4000))

plot(m[,4],t="l")
plot(m[,5],t="l")
plot(m[,6],t="l")


vec = m[,5] - cal[5]
plot(vec,t="l") #, ylim=c(-4000,4000))

max(runsum(vec))/180

plot( runsum(vec)/1106 )

#avg=median(m[,1])

vec = vec-avg

runsum = function(vec) { z=vec; for(i in 2:length(z)) { z[i] = z[i-1]+vec[i] }; return(z); }

m[,3]
plot(runsum(vec+900),t="l")
avg






plot(z,t="l")


bias = 0.92
alpha = 0
beta = 0
gamma = 0
Math.PI = 2*acos(0)

filt = function(acclx,accly, acclz, gyrox,gyroy, gyroz) {
  dt = 1/100
  
  norm = sqrt(acclx ** 2 + accly ** 2 + acclz ** 2);
  
  scale = Math.PI / 2;
  
  alpha <<- alpha + gyroz * dt;
  beta <<- bias * (beta + gyrox * dt) + (1.0 - bias) * (acclx * scale / norm);
  gamma <<- bias * (gamma + gyroy * dt) + (1.0 - bias) * (accly * -scale / norm); 
  
  cat(sprintf("%7.3f %7.3f %7.3f\n", alpha,beta,gamma ));
  
}

mag = function(a,b,c) sqrt(a*a + b*b + c*c)
2000 deg per unit

deg_to_rad = (2*Math.PI/360)


for(i in 1:2200) {
  x = read.table(textConnection(udp_message() ) )
  rotconstant = 2000 * 
  #cat(mag(x[1,1],x[1,2],x[1,3]),"\n")
  
  filt(x[1,1]/earth, x[1,2]/earth, x[1,3]/earth,  x[1,4] * rotconstant,x[1,5] * rotconstant,x[1,6] * rotconstant )
  
}






