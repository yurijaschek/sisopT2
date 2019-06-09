/*****************************************************************************
 *                                                                           *
 *  Funções que realizam a leitura e escrita do subsistema de E/S no disco   *
 *      usado pelo T2FS.                                                     *
 *                                                                           *
 *  Essas funções são realizadas com base nos setores lógicos do disco.      *
 *  Os setores são endereçados através de sua numeração sequencia, a partir  *
 *      de ZERO.                                                             *
 *  O setor lógico tem, sempre, 256 bytes (SECTOR_SIZE).                     *
 *                                                                           *
 *  Versão: 16.2                                                             *
 *                                                                           *
 *****************************************************************************/

#ifndef APIDISK_H
#define APIDISK_H

#define SECTOR_SIZE 256


/*-----------------------------------------------------------------------------
Função: Realiza leitura de um setor lógico do disco.

Entra:  sector -> setor lógico a ser lido, iniciando em ZERO.
        buffer -> ponteiro para a área de memória onde colocar os dados lidos
                  do disco.

Saída:  Se a leitura foi realizada com sucesso, a função retorna "0" (zero).
        Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int read_sector (unsigned int sector, unsigned char *buffer);


/*-----------------------------------------------------------------------------
Função: Realiza escrita de um setor lógico no disco.

Entra:  sector -> setor lógico a ser escrito, iniciando em ZERO.
        buffer -> ponteiro para a área de memória onde estão os dados a serem
                  escritos no disco.

Saída:  Se a escrita foi realizada com sucesso, a função retorna "0" (zero).
        Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int write_sector (unsigned int sector, unsigned char *buffer);


#endif // APIDISK_H
