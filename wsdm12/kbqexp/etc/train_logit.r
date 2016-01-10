train_data<-read.table("/home/alex/projects/kbqexp/results/robust/rank/splits-raw/train.5", header=F);
names(train_data)<-c("qId","expCand","topDocFrac","idf","fanOut","avgColCor","maxColCor","avgTopCor","maxTopCor","avgTopPCor","maxTopPCor","avgQDist","maxQDist","avgPWeight","maxPWeight","map");
attach(train_data);
log_reg<-glm(map~topDocFrac+idf+fanOut+avgColCor+maxColCor+avgTopCor+maxTopCor+avgTopPCor+maxTopPCor+avgQDist+maxQDist+avgPWeight+maxPWeight, family=binomial("logit"));
log_reg