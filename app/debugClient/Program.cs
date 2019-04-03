using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace ConsoleApplication2
{
    class Program
    {
        static void Main(string[] args)
        {
             ASCIIEncoding encoding= new ASCIIEncoding();
            try
            {
                TcpClient client = new TcpClient();

                string ipAddress = Console.ReadLine();
                // 1. connect
                client.Connect(ipAddress, 8888);
                Stream stream = client.GetStream();

                Console.WriteLine("Connected to ESP.");

                while (true)
                {
                    int data = stream.ReadByte();
                    if (data>0)
                    {
                        Console.Write((char)data);
                    }
                }
            }

            catch (Exception ex)
            {
                Console.WriteLine("Error: " + ex);
            }

            Console.Read();
        }
    }
}
