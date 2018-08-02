[f,fs] = gspi;%wavload('serj.wav');%


%[f,fs] = wavload('testFile7.wav');
%[g,a,cfreq,L]=audfilters(fs,numel(f),'uniform','gauss','bwmul',1/2,'spacing',1/4,'redmul',2);
bwmul = 1/3;
[g,a,cfreq,L]=audfilters(fs,numel(f),'fractional','gauss','bwmul',bwmul,'spacing',1/9,'redmul',3);
corig = filterbank(f,g,a);
%coriguni = ufilterbank(f,g,a(1));

tfr = getgausstfr(cfreq,fs,L,'erb','bwmul',bwmul);
cfreq = 2*cfreq/fs;
%[tgrad0,fgrad0] = filterbankphasegrad(f,g,a,L);

% M = size(c,1);
% N = length(c{1});
% c1=zeros(M,N);
% for m=1:M    
%     c1(m,:)=c{m};
% end;
%
% [~,~,~,tgrad1,fgrad1]=filterbankconstphasereal(abs(c1),g,a(1),tfr,cfreq);
% 
% tgrad1 = mat2cell(tgrad1,ones(M,1));
% fgrad1 = mat2cell(fgrad1,ones(M,1));

%av = [a(:,1), ones(M,1)];
% [~,~,~,tgrad,fgrad]=nufbconstphasereal_old(c,g,av,tfr,cfreq);

% %Uniform
% figure(1);
% subplot(131); plotfilterbank(fgrad0,a,'linabs','clim',[0,4500])
% subplot(132); plotfilterbank(fgrad1,a,'linabs','clim',[0,4500])
% subplot(133); plotfilterbank(fgrad,a,'linabs','clim',[0,4500])
% figure(2); 
% subplot(131); plotfilterbank(tgrad0,a,'lin','clim',[-.01,.01])
% subplot(132); plotfilterbank(tgrad1,a,'lin','clim',[-.01,.01])
% subplot(133); plotfilterbank(tgrad,a,'lin','clim',[-.01,.01])

%Nonuniform
disp('About to start')
tic
[cnonuni,newphase,~,tgradnonuni,fgradnonuni]=filterbankconstphase(corig,a,tfr,cfreq,'tol',[1e-1,1e-10],'real');
toc
tic
[cnonuni2,newphase,~,tgradnonuni,fgradnonuni]=filterbankconstphase(corig,a,tfr,cfreq,'tol',[1e-10],'real');
toc
%[cnonuni,newphase,~,tgradnonuni,fgradnonuni]=nufbconstphase(corig,g,a,tfr,cfreq);


%[cuni,newphase,usedmask,tgraduni,fgraduni]=filterbankconstphasereal(abs(coriguni).',g,a(1),tfr,cfreq,'tol',1e-3);
% figure(1);
% subplot(121); plotfilterbank(fgrad0,a,'linabs','clim',[0,400])
% subplot(122); plotfilterbank(fgrad,a,'linabs','clim',[0,400])
% figure(2); 
% subplot(121); plotfilterbank(tgrad0,a,'lin','clim',[-.01,.01])
% subplot(122); plotfilterbank(tgrad,a,'lin','clim',[-.01,.01])

fhat = ifilterbankiter(cnonuni,g,a,'cg');

%figure(1);plotfilterbankphasediff(coriguni,cuni.',1e-3,a)
figure(1);plotfilterbankphasediff(corig,cnonuni,1e-6,a)
figure(2);plotfilterbankphasediff(corig,cnonuni2,1e-6,a)