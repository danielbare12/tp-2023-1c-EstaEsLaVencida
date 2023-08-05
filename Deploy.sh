#!/bin/bash

git_USUARIO = "Juan-Polito"
git_PASSWORD = "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"

testfunction(){

   cd $1
   git clone https://github.com/sisoputnfrba/so-commons-library.git$1
   git_USUARIO $1
   git_PASSWORD $1
   echo "Se instalaron las commons"

   cd so-commons-library/ $1
   make all $1
   echo "Se generaron las estructuras de las COMMONS"


   cd $1
   cd tp-2023-1c-EstaEsLaVencida/compartido/Debug/ $1
   make all $1
   cd .. $1
   cd src/ $1
   make install $1
   echo "Se generaron las estrucuras de COMPARTIDO"

   cd $1
   cd tp-2023-1c-EstaEsLaVencida/memoria/Debug/ $1
   make all $1

   cd $1
   cd tp-2023-1c-EstaEsLaVencida/cpu/Debug/ $1
   make all $1

   cd $1
   cd tp-2023-1c-EstaEsLaVencida/fileSystem/Debug/ $1
   make all $1

   cd $1
   cd tp-2023-1c-EstaEsLaVencida/kernel/Debug/ $1
   make all $1

   cd $1
   cd tp-2023-1c-EstaEsLaVencida/consola/Debug/ $1
   make all $1

   echo "todo listo para ejecucion"
}
testfunction
