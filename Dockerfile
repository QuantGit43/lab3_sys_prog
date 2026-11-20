FROM bslab-img
RUN apt-get update && apt-get install -y --no-install-recommends flex bison make && rm -rf /var/lib/apt/lists/*
WORKDIR /w
